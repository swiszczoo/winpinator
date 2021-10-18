#include "transfer_manager.hpp"

#include "../globals.hpp"
#include "auth_manager.hpp"
#include "notification_accept_files.hpp"
#include "service_utils.hpp"

#include <wx/filename.h>

#include <memory>

namespace srv
{

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
    m_lastId++;

    checkTransferDiskSpace( transfer );
    checkTransferMustOverwrite( transfer );
    setTransferTimestamp( transfer );
    sendNotifications( remoteId, transfer );

    m_transfers[remoteId].push_back( transfer );

    m_srv->notifyObservers( [this, &transfer]( srv::IServiceObserver* observer )
        { observer->onAddTransfer( transfer ); } );
}

void TransferManager::replyAllowTransfer( const std::string& remoteId,
    int transferId, bool allow )
{
    std::lock_guard<std::mutex> guard( m_mtx );

    TransferOp& op = getTransferInfo( remoteId, transferId );

    if ( allow )
    {
        if ( !op.outcoming && op.status == OpStatus::WAITING_PERMISSION )
        {
            processStartOrResumeTransfer( remoteId, op );
        }
    }
}

std::vector<TransferOp> TransferManager::getTransfersForRemote(
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

TransferOp& TransferManager::getTransferInfo( const std::string& remoteId, 
    int transferId )
{
    if ( m_transfers.find( remoteId ) != m_transfers.end() )
    {
        auto& transferList = m_transfers[remoteId];

        for ( TransferOp& op : transferList )
        {
            if ( op.id == transferId )
            {
                return op;
            }
        }
    }

    return m_empty;
}

void TransferManager::processStartOrResumeTransfer( const std::string& remoteId,
    TransferOp& op )
{
    wxLogDebug( "TransferManager: processing start/resume transfer" );

    RemoteInfoPtr info = m_remoteMgr->getRemoteInfo( remoteId );

    if ( !info )
    {
        wxLogDebug( "TransferManager: remote not found! ignoring!" );
        return;
    }

    std::lock_guard<std::mutex> guard( info->mutex );

    std::shared_ptr<grpc::ClientContext> ctx
        = std::make_shared<grpc::ClientContext>();
    std::shared_ptr<OpInfo> request = std::make_shared<OpInfo>();

    request->set_ident( AuthManager::get()->getIdent() );
    request->set_timestamp( op.startTime );
    request->set_readable_name( Utils::getHostname() );

    // TODO: we can also request not to use compression
    request->set_use_compression( op.useCompression );

    std::shared_ptr<StartTransferReactor> reactor 
        = std::make_shared<StartTransferReactor>();
    reactor->setInstance( reactor );
    reactor->setRefs( ctx, request );
    
    wxLogDebug( "TransferManager: starting transfer!" );
    op.status = OpStatus::TRANSFERRING;

    info->stub->experimental_async()->StartTransfer( ctx.get(), request.get(),
        reactor.get() );
    reactor->start();
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

        StartRead( &m_chunk );
    }
}

};
