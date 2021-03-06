#include "warp_service_impl.hpp"

#include "../thread_name.hpp"
#include "service_utils.hpp"

#include <grpcpp/grpcpp.h>
#include <grpcpp/security/server_credentials.h>

#include <wx/log.h>

namespace srv
{

const int WarpServer::DEFAULT_SERVICE_PORT = 52000;
const std::string WarpServer::EMPTY = "";

WarpServiceImpl::WarpServiceImpl( std::shared_ptr<RemoteManager> remoteMgr, 
    std::shared_ptr<TransferManager> transferMgr,
    std::shared_ptr<std::string> avatar )
    : m_remoteMgr( remoteMgr )
    , m_transferMgr( transferMgr )
    , m_avatar( avatar )
{
}

Status WarpServiceImpl::Ping( grpc::ServerContext* context, 
    const LookupName* request, VoidType* response )
{
    wxLogDebug( "Server Ping: from %s", request->readable_name() );

    const std::string& id = request->id();

    if ( !m_remoteMgr->isHostAvailable( id ) )
    {
        wxLogDebug( "Server Ping: ping is from unknown remote "
            "(or not fully online yet)" );
    }

    response->set_dummy( 0 );
    return Status::OK;
}

Status WarpServiceImpl::CheckDuplexConnection( grpc::ServerContext* context, 
    const LookupName* request, HaveDuplex* response )
{
    wxLogDebug( "Server RPC: CheckDuplexConnection from '%s'",
        request->readable_name() );

    bool responseFlag = false;
    
    auto remote = m_remoteMgr->getRemoteInfo( request->id() );
    if ( remote )
    {
        responseFlag = ( remote->state == RemoteStatus::AWAITING_DUPLEX
            || remote->state == RemoteStatus::ONLINE );
    }

    response->set_response( responseFlag );

    return Status::OK;
}

Status WarpServiceImpl::WaitingForDuplex( grpc::ServerContext* context,
    const LookupName* request, HaveDuplex* response )
{
    wxLogDebug( "Server RPC: WaitingForDuplex from '%s' (api v2)",
        request->readable_name() );

    const int MAX_TRIES = 20;
    int i = 0;
    bool responseFlag = false;

    // try for ~5 seconds( the caller aborts at 4 )
    // we actually wait for 10 seconds
    while ( i < MAX_TRIES )
    {
        responseFlag = false;

        auto remote = m_remoteMgr->getRemoteInfo( request->id() );
        if ( remote )
        {
            responseFlag = ( remote->state == RemoteStatus::AWAITING_DUPLEX
                || remote->state == RemoteStatus::ONLINE );
        }

        if ( responseFlag )
        {
            break;
        }
        else
        {
            i++;

            if ( i == MAX_TRIES )
            {
                return Status( grpc::StatusCode::DEADLINE_EXCEEDED,
                    "Server timed out while waiting for his corresponding"
                    " remote to connect back to you." );
            }
            
            std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
        }
    }

    response->set_response( responseFlag );
    return Status::OK;
}

Status WarpServiceImpl::GetRemoteMachineInfo( grpc::ServerContext* context,
    const LookupName* request, RemoteMachineInfo* response )
{
    wxLogDebug( "Server RPC: GetRemoteMachineInfo from '%s'",
        request->readable_name() );

    std::wstring fullName = Utils::getUserFullName();
    std::wstring shortName = Utils::getUserShortName();

    response->set_display_name( wxString( fullName ).utf8_string() );
    response->set_user_name( wxString( shortName ).utf8_string() );

    return Status::OK;
}

Status WarpServiceImpl::GetRemoteMachineAvatar( grpc::ServerContext* context,
    const LookupName* request, grpc::ServerWriter<RemoteMachineAvatar>* writer )
{
    if ( !m_avatar || m_avatar->empty() )
    {
        return Status( grpc::StatusCode::NOT_FOUND, 
            "Avatar file not available!" );
    }

    const char* data = m_avatar->data();
    const size_t BLOCK_SIZE = 1024 * 1024; // 1 MB
    size_t pos = 0;
    size_t length = m_avatar->size();

    while ( true )
    {
        size_t thisBlockSize = std::min( BLOCK_SIZE, length - pos );
        if ( thisBlockSize == 0 )
        {
            break;
        }

        RemoteMachineAvatar chunk;
        chunk.set_avatar_chunk( data + pos, thisBlockSize );
        writer->Write( chunk );

        pos += thisBlockSize;
    }

    return Status::OK;
}

Status WarpServiceImpl::ProcessTransferOpRequest( grpc::ServerContext* context,
    const TransferOpRequest* request, VoidType* response )
{
    if ( !request->has_info() )
    {
        return Status( grpc::StatusCode::INVALID_ARGUMENT, "Missing OpInfo" );
    }

    const OpInfo& info = request->info();

    wxLogDebug( "Server RPC: ProcessTransferOpRequest from '%s'",
        info.readable_name() );

    const std::string& id = info.ident();

    if ( !m_remoteMgr->isHostAvailable( id ) )
    {
        wxLogDebug( "Received transfer op request for unknown op" );
        return Status( grpc::StatusCode::PERMISSION_DENIED,
            "Invalid sender ident" );
    }

    for ( TransferOpPtr existingOp : m_transferMgr->getTransfersForRemote( id ) )
    {
        std::unique_lock<std::mutex> lock( *existingOp->mutex );

        if ( existingOp->startTime == info.timestamp() )
        {
            existingOp->useCompression = info.use_compression();
            existingOp->status = OpStatus::WAITING_PERMISSION;

            lock.unlock();

            m_transferMgr->registerTransfer( id, *existingOp, false );
            return Status::OK;
        }
    }

    TransferOp op;
    op.outcoming = false;
    op.startTime = info.timestamp();
    op.senderNameUtf8 = request->sender_name();
    op.receiverUtf8 = request->receiver();
    op.receiverNameUtf8 = request->receiver_name();
    op.status = OpStatus::WAITING_PERMISSION;
    op.totalSize = request->size();
    op.totalCount = request->count();
    op.mimeIfSingleUtf8 = request->mime_if_single();
    op.nameIfSingleUtf8 = request->name_if_single();
    op.useCompression = info.use_compression();

    for ( std::string basename : request->top_dir_basenames() )
    {
        op.topDirBasenamesUtf8.push_back( basename );
    }

    m_transferMgr->registerTransfer( id, op, true );

    return Status::OK;
}

Status WarpServiceImpl::StartTransfer( grpc::ServerContext* context,
    const OpInfo* request, grpc::ServerWriter<FileChunk>* writer )
{
    wxLogDebug( "Server RPC: StartTransfer from '%s'",
        request->readable_name() );

    const std::string& id = request->ident();
    TransferOpPub op = m_transferMgr->getOp( id, request->timestamp() );

    if ( !m_remoteMgr->isHostAvailable( id ) || op.id == -1 )
    {
        wxLogDebug( "Received start transfer request for unknown op" );
        return Status( grpc::StatusCode::PERMISSION_DENIED,
            "Invalid sender ident" );
    }

    bool result = m_transferMgr->handleOutcomingTransfer( 
        id, op.id, writer, request->use_compression() );

    if ( result ) {
        return Status::OK;
    }
    return Status( grpc::StatusCode::CANCELLED, "Internal error occured" );
}

Status WarpServiceImpl::CancelTransferOpRequest( grpc::ServerContext* context,
    const OpInfo* request, VoidType* response )
{
    wxLogDebug( "Server RPC: ProcessTransferOpRequest from '%s'",
        request->readable_name() );

    const std::string& id = request->ident();

    if ( !m_remoteMgr->isHostAvailable( id )
        || m_transferMgr->getOp( id, request->timestamp() ).id == -1 )
    {
        wxLogDebug( "Received cancel transfer op request for unknown op" );
        return Status( grpc::StatusCode::PERMISSION_DENIED,
            "Invalid sender ident" );
    }

    m_transferMgr->cancelTransferRequest( id, request->timestamp() );
    return Status::OK;
}

Status WarpServiceImpl::StopTransfer( grpc::ServerContext* context,
    const StopInfo* request, VoidType* response )
{
    if ( !request->has_info() )
    {
        return Status( grpc::StatusCode::INVALID_ARGUMENT, "Missing OpInfo" );
    }

    const OpInfo& info = request->info();

    wxLogDebug( "Server RPC: StopTransfer from '%s'",
        request->info().readable_name() );

    const std::string& id = info.ident();
    TransferOpPub op = m_transferMgr->getOp( id, info.timestamp() );

    if ( !m_remoteMgr->isHostAvailable( id ) || op.id == -1 )
    {
        wxLogDebug( "Received cancel transfer op request for unknown op" );
        return Status( grpc::StatusCode::PERMISSION_DENIED,
            "Invalid sender ident" );
    }

    bool error = request->error();
    m_transferMgr->stopTransfer( id, op.id, error );

    return Status::OK;
}

//
// Server implementation

WarpServer::WarpServer()
    : m_port( WarpServer::DEFAULT_SERVICE_PORT )
    , m_privKey( "" )
    , m_pubKey( "" )
    , m_avatar( nullptr )
    , m_remoteMgr( nullptr )
    , m_transferMgr( nullptr )
    , m_server( nullptr )
{
}

WarpServer::~WarpServer()
{
    stopServer();
}

void WarpServer::setPort( uint16_t port )
{
    m_port = port;
}

uint16_t WarpServer::getPort() const
{
    return m_port;
}

void WarpServer::setPemPrivateKey( const std::string& pem )
{
    m_privKey = pem;
}

const std::string& WarpServer::getPemPrivateKey() const
{
    return m_privKey;
}

void WarpServer::setPemCertificate( const std::string& pem )
{
    m_pubKey = pem;
}

const std::string& WarpServer::getPemCertificate() const
{
    return m_pubKey;
}

void WarpServer::setAvatarBytes( const std::string& bytes )
{
    m_avatar = std::make_shared<std::string>( bytes );
}

const std::string& WarpServer::getAvatarBytes() const
{
    if ( !m_avatar )
    {
        return WarpServer::EMPTY;
    }

    return *m_avatar;
}

void WarpServer::setRemoteManager( std::shared_ptr<RemoteManager> mgr )
{
    m_remoteMgr = mgr;
}

std::shared_ptr<RemoteManager> WarpServer::getRemoteManager() const
{
    return m_remoteMgr;
}

void WarpServer::setTransferManager( std::shared_ptr<TransferManager> mgr )
{
    m_transferMgr = mgr;
}

std::shared_ptr<TransferManager> WarpServer::getTransferManager() const
{
    return m_transferMgr;
}

bool WarpServer::startServer()
{
    if ( m_server )
    {
        // Server is already running
        return false;
    }

    std::promise<bool> startedPromise;
    std::future<bool> startedFuture = startedPromise.get_future();

    m_thread = std::thread( std::bind( &WarpServer::threadMain, this,
        m_port, m_privKey, m_pubKey, m_remoteMgr, m_transferMgr, m_avatar,
        std::ref( startedPromise ) ) );

    // Wait for the server pointer to become valid
    return startedFuture.get();
}

bool WarpServer::stopServer()
{
    if ( !m_server )
    {
        if ( m_thread.joinable() )
        {
            m_thread.join();
        }

        // Server is not running
        return false;
    }

    m_server->Shutdown();

    if ( m_thread.joinable() )
    {
        m_thread.join();
    }

    // Ensure that pointer is no longer valid
    m_server = nullptr;

    return true;
}

int WarpServer::threadMain( uint16_t port, std::string priv, std::string pub,
    std::shared_ptr<RemoteManager> remoteMgr, 
    std::shared_ptr<TransferManager> transferMgr, 
    std::shared_ptr<std::string> avatar, 
    std::promise<bool>& startProm )
{
    setThreadName( "Warp service worker (gRPC server controller)" );

    std::string address = "0.0.0.0:" + std::to_string( port );
    WarpServiceImpl service( remoteMgr, transferMgr, avatar );

    grpc::SslServerCredentialsOptions::PemKeyCertPair pair;
    pair.private_key = priv;
    pair.cert_chain = pub;

    grpc::SslServerCredentialsOptions credOpts( 
        GRPC_SSL_DONT_REQUEST_CLIENT_CERTIFICATE );
    credOpts.pem_key_cert_pairs.push_back( pair );
    credOpts.pem_root_certs = pub;

    auto credentials = grpc::SslServerCredentials( credOpts );

    grpc::ServerBuilder builder;
    builder.AddListeningPort( address, credentials );
    builder.AddChannelArgument( GRPC_ARG_KEEPALIVE_TIME_MS, 10 * 1000 );
    builder.AddChannelArgument( GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 5 * 1000 );
    builder.AddChannelArgument( GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, true );
    builder.AddChannelArgument( GRPC_ARG_HTTP2_MAX_PINGS_WITHOUT_DATA, 0 );
    builder.AddChannelArgument( 
        GRPC_ARG_HTTP2_MIN_RECV_PING_INTERVAL_WITHOUT_DATA_MS, 5 * 1000 );
    builder.AddChannelArgument(
        GRPC_ARG_HTTP2_MIN_SENT_PING_INTERVAL_WITHOUT_DATA_MS, 10 * 1000 );
    builder.RegisterService( &service );

    m_server = std::unique_ptr<grpc::Server>( builder.BuildAndStart() );

    // Unlock the controlling thread
    if ( m_server )
    {
        startProm.set_value( true );
        m_server->Wait();
    }
    else
    {
        startProm.set_value( false );
    }


    return EXIT_SUCCESS;
}

};
