#include "transfer_manager.hpp"

#include "../globals.hpp"
#include "auth_manager.hpp"
#include "notification_accept_files.hpp"
#include "notification_transfer_failed.hpp"
#include "service_utils.hpp"

#include <wx/filename.h>

#include <memory>

namespace srv
{

const long long TransferManager::PROGRESS_FREQ_MILLIS = 100;

TransferManager::TransferManager( ObservableService* service )
    : m_running( ATOMIC_VAR_INIT( true ) )
    , m_remoteMgr( nullptr )
    , m_srv( service )
    , m_lastId( 0 )
    , m_compressionLevel( 0 )
    , m_empty()
{
}

TransferManager::~TransferManager()
{
    if ( m_running )
    {
        stop();
    }
}

void TransferManager::setOutputPath( const std::wstring& path )
{
    std::lock_guard<std::mutex> guard( m_mtx );

    m_outputPath = path;
}

std::wstring TransferManager::getOutputPath()
{
    std::lock_guard<std::mutex> guard( m_mtx );

    return m_outputPath;
}

void TransferManager::setRemoteManager( std::shared_ptr<RemoteManager> ptr )
{
    std::lock_guard<std::mutex> guard( m_mtx );

    m_remoteMgr = ptr;
}

RemoteManager* TransferManager::getRemoteManager()
{
    std::lock_guard<std::mutex> guard( m_mtx );

    return m_remoteMgr.get();
}

void TransferManager::setCompressionLevel( int level )
{
    std::lock_guard<std::mutex> guard( m_mtx );

    m_compressionLevel = level;
}

int TransferManager::getCompressionLevel()
{
    std::lock_guard<std::mutex> guard( m_mtx );

    return m_compressionLevel;
}

void TransferManager::stop()
{
    std::lock_guard<std::mutex> guard( m_mtx );
}

void TransferManager::registerTransfer( const std::string& remoteId,
    TransferOp& transfer, bool firstTry )
{
    std::lock_guard<std::mutex> guard( m_mtx );

    if ( firstTry )
    {
        transfer.id = m_lastId;
        transfer.mutex = std::make_shared<std::mutex>();
        m_lastId++;
    }

    std::lock_guard<std::mutex> lock( *transfer.mutex );

    transfer.meta.sentBytes = 0;

    checkTransferDiskSpace( transfer );
    checkTransferMustOverwrite( transfer );
    setTransferTimestamp( transfer );
    setUpPauseLock( transfer );
    sendNotifications( remoteId, transfer );

    if ( firstTry )
    {
        m_transfers[remoteId].push_back( std::make_shared<TransferOp>( transfer ) );
    }

    m_srv->notifyObservers(
        [this, &remoteId, &transfer]( srv::IServiceObserver* observer )
        { observer->onAddTransfer( remoteId, transfer ); } );
}

void TransferManager::replyAllowTransfer( const std::string& remoteId,
    int transferId, bool allow )
{
    std::lock_guard<std::mutex> guard( m_mtx );

    TransferOpPtr op = getTransferInfo( remoteId, transferId );

    if ( !op )
    {
        return;
    }

    std::unique_lock<std::mutex> lock( *op->mutex );
    if ( op->status == OpStatus::WAITING_PERMISSION )
    {
        if ( allow && !op->outcoming )
        {
            lock.unlock();
            processStartTransfer( remoteId, op );
        }
        else if ( !allow )
        {
            lock.unlock();
            processDeclineTransfer( remoteId, op );
        }
    }
}

void TransferManager::cancelTransferRequest( const std::string& remoteId,
    time_t transferId )
{
    std::lock_guard<std::mutex> guard( m_mtx );

    TransferOpPtr op = getTransferByTimestamp( remoteId, transferId );
    if ( !op )
    {
        return;
    }

    std::lock_guard<std::mutex> opLock( *op->mutex );
    if ( op->outcoming )
    {
        op->status = OpStatus::CANCELLED_PERMISSION_BY_RECEIVER;
    }
    else
    {
        op->status = OpStatus::CANCELLED_PERMISSION_BY_SENDER;
    }

    sendStatusUpdateNotification( remoteId, op );
    // TODO: finish transfer
}

void TransferManager::resumeTransfer( const std::string& remoteId,
    int transferId )
{
    std::lock_guard<std::mutex> guard( m_mtx );

    TransferOpPtr op = getTransferInfo( remoteId, transferId );
    bool opPaused = false;

    {
        std::lock_guard<std::mutex> lck( *op->mutex );
        opPaused = op->status == OpStatus::PAUSED;
    }

    if ( !op || !opPaused )
    {
        return;
    }

    {
        std::lock_guard<std::mutex> lck( op->intern.pauseLock->mutex );
        op->intern.pauseLock->value = false;
        op->intern.pauseLock->condVar.notify_all();
    }

    {
        std::lock_guard<std::mutex> lck( *op->mutex );
        op->status = OpStatus::TRANSFERRING;

        sendStatusUpdateNotification( remoteId, op );
    }
}

void TransferManager::pauseTransfer( const std::string& remoteId,
    int transferId )
{
    std::lock_guard<std::mutex> guard( m_mtx );

    TransferOpPtr op = getTransferInfo( remoteId, transferId );
    bool opRunning = false;

    {
        std::lock_guard<std::mutex> lck( *op->mutex );
        opRunning = op->status == OpStatus::TRANSFERRING;
    }

    if ( !op || !opRunning )
    {
        return;
    }

    {
        std::lock_guard<std::mutex> lck( op->intern.pauseLock->mutex );
        op->intern.pauseLock->value = true;
        op->intern.pauseLock->condVar.notify_all();
    }

    {
        std::lock_guard<std::mutex> lck( *op->mutex );
        op->status = OpStatus::PAUSED;

        sendStatusUpdateNotification( remoteId, op );
    }
}

void TransferManager::failAll( const std::string& remoteId )
{
    std::lock_guard<std::mutex> guard( m_mtx );

    bool someFailed = false;
    std::wstring senderName;
    if ( m_transfers.find( remoteId ) != m_transfers.end() )
    {
        for ( auto& op : m_transfers[remoteId] )
        {
            std::lock_guard<std::mutex> lock( *op->mutex );
            if ( op->status != OpStatus::FAILED )
            {
                someFailed = true;
                senderName = wxString::FromUTF8( op->senderNameUtf8 ).ToStdWstring();
            }

            op->status = OpStatus::FAILED;

            sendStatusUpdateNotification( remoteId, op );
        }
    }

    if ( someFailed )
    {
        auto notif = std::make_shared<TransferFailedNotification>( remoteId );
        notif->setSenderFullName( senderName );

        Event evnt;
        evnt.type = EventType::SHOW_TOAST_NOTIFICATION;
        evnt.eventData.toastData = notif;

        Globals::get()->getWinpinatorServiceInstance()->postEvent( evnt );
    }
}

std::mutex& TransferManager::getMutex()
{
    return m_mtx;
}

std::vector<TransferOpPtr> TransferManager::getTransfersForRemote(
    const std::string& remoteId )
{
    std::lock_guard<std::mutex> guard( m_mtx );

    if ( m_transfers.find( remoteId ) != m_transfers.end() )
    {
        return m_transfers[remoteId];
    }

    return {};
}

void TransferManager::checkTransferDiskSpace( TransferOp& op )
{
    wxDiskspaceSize_t totalSpace;
    wxDiskspaceSize_t freeSpace;

    wxGetDiskSpace( m_outputPath, &totalSpace, &freeSpace );

    op.meta.notEnoughSpace = freeSpace < op.totalSize;
}

void TransferManager::checkTransferMustOverwrite( TransferOp& op )
{
    bool mustOverwrite = false;

    for ( std::string elementUtf8 : op.topDirBasenamesUtf8 )
    {
        wxString element = wxString::FromUTF8( elementUtf8 );

        wxFileName fname( m_outputPath, element );

        if ( fname.Exists() )
        {
            mustOverwrite = true;
            break;
        }
    }

    op.meta.mustOverwrite = mustOverwrite;
}

void TransferManager::setTransferTimestamp( TransferOp& op )
{
    op.meta.localTimestamp = time( NULL );
    op.intern.lastProgressUpdate = std::chrono::steady_clock::now();
}

void TransferManager::setUpPauseLock( TransferOp& op )
{
    op.intern.pauseLock = std::make_shared<EventLock>();
    op.intern.pauseLock->value = false;
}

void TransferManager::sendNotifications( const std::string& remoteId,
    TransferOp& op )
{
    if ( !op.outcoming )
    {
        auto notif = std::make_shared<AcceptFilesNotification>( remoteId, op.id );
        notif->setSenderFullName(
            wxString::FromUTF8( op.senderNameUtf8 ).ToStdWstring() );
        notif->setElementCount( op.topDirBasenamesUtf8.size() );
        notif->setIsSingleFolder( op.mimeIfSingleUtf8 == "inode/directory" );
        if ( op.topDirBasenamesUtf8.size() == 1 )
        {
            notif->setSingleElementName(
                wxString::FromUTF8( op.topDirBasenamesUtf8[0] ).ToStdWstring() );
        }

        Event evnt;
        evnt.type = EventType::SHOW_TOAST_NOTIFICATION;
        evnt.eventData.toastData = notif;

        Globals::get()->getWinpinatorServiceInstance()->postEvent( evnt );
    }
}

TransferOpPtr TransferManager::getTransferInfo( const std::string& remoteId,
    int transferId )
{
    if ( m_transfers.find( remoteId ) != m_transfers.end() )
    {
        auto& transferList = m_transfers[remoteId];

        for ( TransferOpPtr op : transferList )
        {
            if ( op->id == transferId )
            {
                return op;
            }
        }
    }

    return nullptr;
}

TransferOpPtr TransferManager::getTransferByTimestamp(
    const std::string& remoteId, time_t timestamp )
{
    if ( m_transfers.find( remoteId ) != m_transfers.end() )
    {
        auto& transferList = m_transfers[remoteId];

        for ( TransferOpPtr op : transferList )
        {
            if ( op->startTime == timestamp )
            {
                return op;
            }
        }
    }

    return nullptr;
}

void TransferManager::processStartTransfer( const std::string& remoteId,
    TransferOpPtr op )
{
    wxLogDebug( "TransferManager: processing start transfer" );

    {
        RemoteInfoPtr info = m_remoteMgr->getRemoteInfo( remoteId );

        if ( !info )
        {
            wxLogDebug( "TransferManager: remote not found! ignoring!" );
            return;
        }

        std::lock_guard<std::mutex> guard( info->mutex );

        std::shared_ptr<grpc::ClientContext> ctx
            = std::make_shared<grpc::ClientContext>();

        std::shared_ptr<OpInfo> request = std::make_shared<OpInfo>(
            convertOpToOpInfo( op, m_compressionLevel > 0 ) );

        std::shared_ptr<StartTransferReactor> reactor
            = std::make_shared<StartTransferReactor>();
        reactor->setInstance( reactor );
        reactor->setRefs( ctx, request );
        reactor->setTransferPtr( op );
        reactor->setRemoteId( remoteId );
        reactor->setManager( this );

        wxLogDebug( "TransferManager: starting transfer!" );

        info->stub->experimental_async()->StartTransfer( ctx.get(), request.get(),
            reactor.get() );
        reactor->start();
    }

    std::lock_guard<std::mutex> lck( *op->mutex );
    op->status = OpStatus::TRANSFERRING;
    sendStatusUpdateNotification( remoteId, op );
}

void TransferManager::processDeclineTransfer( const std::string& remoteId,
    TransferOpPtr op )
{
    wxLogDebug( "TransferManager: rejecting transfer" );

    {
        RemoteInfoPtr info = m_remoteMgr->getRemoteInfo( remoteId );

        if ( !info )
        {
            wxLogDebug( "TransferManager: remote not found! ignoring!" );
            return;
        }

        std::lock_guard<std::mutex> guard( info->mutex );

        grpc::ClientContext ctx;
        ctx.set_deadline( std::chrono::system_clock::now()
            + std::chrono::seconds( 3 ) );
        OpInfo request = convertOpToOpInfo( op, m_compressionLevel > 0 );
        VoidType response;

        if ( !info->stub->CancelTransferOpRequest( &ctx, request, &response ).ok() )
        {
            wxLogDebug( "TransferManager: CancelTransferOpRequest failed" );
        }
    }

    std::lock_guard<std::mutex> lck( *op->mutex );
    if ( op->outcoming )
    {
        op->status = OpStatus::CANCELLED_PERMISSION_BY_SENDER;
    }
    else
    {
        op->status = OpStatus::CANCELLED_PERMISSION_BY_RECEIVER;
    }
    sendStatusUpdateNotification( remoteId, op );
}

OpInfo TransferManager::convertOpToOpInfo( const TransferOpPtr op,
    bool compressionEnabled ) const
{
    std::lock_guard<std::mutex> guard( *op->mutex );

    OpInfo info;

    info.set_ident( AuthManager::get()->getIdent() );
    info.set_timestamp( op->startTime );
    info.set_readable_name( Utils::getHostname() );
    info.set_use_compression( op->useCompression && compressionEnabled );

    return info;
}

void TransferManager::sendStatusUpdateNotification( const std::string& remoteId,
    const TransferOpPtr op )
{
    m_srv->notifyObservers( [this, &remoteId, &op]( IServiceObserver* observer )
        { observer->onUpdateTransfer( remoteId, *op ); } );
}

};
