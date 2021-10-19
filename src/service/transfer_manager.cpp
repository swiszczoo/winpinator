#include "transfer_manager.hpp"

#include "../globals.hpp"
#include "auth_manager.hpp"
#include "notification_accept_files.hpp"
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

void TransferManager::stop()
{
    std::lock_guard<std::mutex> guard( m_mtx );
}

void TransferManager::registerTransfer( const std::string& remoteId,
    TransferOp& transfer )
{
    std::lock_guard<std::mutex> guard( m_mtx );

    transfer.id = m_lastId;
    transfer.meta.sentBytes = 0;
    m_lastId++;

    checkTransferDiskSpace( transfer );
    checkTransferMustOverwrite( transfer );
    setTransferTimestamp( transfer );
    sendNotifications( remoteId, transfer );

    m_transfers[remoteId].push_back( std::make_shared<TransferOp>( transfer ) );

    m_srv->notifyObservers( [this, &transfer]( srv::IServiceObserver* observer )
        { observer->onAddTransfer( transfer ); } );
}

void TransferManager::replyAllowTransfer( const std::string& remoteId,
    int transferId, bool allow )
{
    std::lock_guard<std::mutex> guard( m_mtx );

    TransferOpPtr op = getTransferInfo( remoteId, transferId );

    if ( allow )
    {
        if ( op && !op->outcoming && op->status == OpStatus::WAITING_PERMISSION )
        {
            processStartOrResumeTransfer( remoteId, op );
        }
    }
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

void TransferManager::sendNotifications( const std::string& remoteId,
    TransferOp& op )
{
    if ( !op.outcoming )
    {
        // TODO: add failure case notification

        auto notif = std::make_shared<AcceptFilesNotification>( remoteId, op.id );
        notif->setSenderFullName( wxString::FromUTF8( op.senderNameUtf8 ) );
        notif->setElementCount( op.topDirBasenamesUtf8.size() );
        notif->setIsSingleFolder( op.mimeIfSingleUtf8 == "inode/directory" );
        if ( op.topDirBasenamesUtf8.size() == 1 )
        {
            notif->setSingleElementName(
                wxString::FromUTF8( op.topDirBasenamesUtf8[0] ) );
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

void TransferManager::processStartOrResumeTransfer( const std::string& remoteId,
    TransferOpPtr op )
{
    wxLogDebug( "TransferManager: processing start/resume transfer" );

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

        // TODO: we can also request not to use compression
        std::shared_ptr<OpInfo> request = std::make_shared<OpInfo>(
            convertOpToOpInfo( op, true ) );

        std::shared_ptr<StartTransferReactor> reactor
            = std::make_shared<StartTransferReactor>();
        reactor->setInstance( reactor );
        reactor->setRefs( ctx, request );
        reactor->setTransferPtr( op );
        reactor->setManager( this );

        wxLogDebug( "TransferManager: starting transfer!" );
        op->status = OpStatus::TRANSFERRING;

        info->stub->experimental_async()->StartTransfer( ctx.get(), request.get(),
            reactor.get() );
        reactor->start();
    }

    sendStatusUpdateNotification( op );
}

OpInfo TransferManager::convertOpToOpInfo( const TransferOpPtr op,
    bool compressionEnabled ) const
{
    OpInfo info;

    info.set_ident( AuthManager::get()->getIdent() );
    info.set_timestamp( op->startTime );
    info.set_readable_name( Utils::getHostname() );
    info.set_use_compression( op->useCompression && compressionEnabled );

    return info;
}

void TransferManager::sendStatusUpdateNotification( const TransferOpPtr op )
{
    m_srv->notifyObservers( [this, &op]( IServiceObserver* observer )
        { observer->onUpdateTransfer( *op ); } );
}

//
// StartTransferReactor functions

void TransferManager::StartTransferReactor::setInstance(
    std::shared_ptr<StartTransferReactor> selfPtr )
{
    m_selfPtr = selfPtr;
}

void TransferManager::StartTransferReactor::setRefs(
    std::shared_ptr<grpc::ClientContext> ref1, std::shared_ptr<OpInfo> ref2 )
{
    m_clientCtx = ref1;
    m_request = ref2;
}

void TransferManager::StartTransferReactor::setTransferPtr( TransferOpPtr ptr )
{
    m_transfer = ptr;
    m_useCompression = ptr->useCompression;
}

void TransferManager::StartTransferReactor::setManager( TransferManager* mgr )
{
    m_mgr = mgr;
}

void TransferManager::StartTransferReactor::start()
{
    StartRead( &m_chunk );
    StartCall();
}

void TransferManager::StartTransferReactor::OnDone( const grpc::Status& s )
{
    m_selfPtr = nullptr;
}

void TransferManager::StartTransferReactor::OnReadDone( bool ok )
{
    if ( ok )
    {
        wxLogDebug( "StartTransferReactor: Successfully received file chunk (name=%s, type=%d, mode=%d, size=%d)",
            m_chunk.relative_path(), (int)m_chunk.file_type(), (int)m_chunk.file_mode(),
            (int)m_chunk.chunk().size() );

        long long chunkSize = m_chunk.chunk().size();

        if ( m_useCompression )
        {
            std::string decompressed = m_compressor.decompress( m_chunk.chunk() );
            chunkSize = decompressed.size();
        }

        updateProgress( chunkSize );

        StartRead( &m_chunk );
    }
}

void TransferManager::StartTransferReactor::updateProgress( long long chunkBytes )
{
    using namespace std::chrono;
    std::lock_guard<std::mutex> lock( m_mgr->m_mtx );

    m_transfer->meta.sentBytes += chunkBytes;

    auto currentTime = std::chrono::steady_clock::now();
    long long millisElapsed = duration_cast<milliseconds>( 
        currentTime - m_transfer->intern.lastProgressUpdate ).count();

    if ( millisElapsed > TransferManager::PROGRESS_FREQ_MILLIS )
    {
        m_mgr->sendStatusUpdateNotification( m_transfer );

        m_transfer->intern.lastProgressUpdate = currentTime;
    }
}

};
