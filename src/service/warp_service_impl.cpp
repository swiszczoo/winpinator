#include "warp_service_impl.hpp"

#include <grpcpp/grpcpp.h>
#include <grpcpp/security/server_credentials.h>

#include <wx/log.h>

namespace srv
{

const int WarpServer::DEFAULT_SERVICE_PORT = 52000;

WarpServiceImpl::WarpServiceImpl( std::shared_ptr<RemoteManager> mgr )
    : m_manager( mgr )
{
}

Status WarpServiceImpl::Ping( grpc::ServerContext* context, 
    const LookupName* request, VoidType* response )
{
    wxLogDebug( "Server Ping: from %s", request->readable_name() );

    const std::string& id = request->id();

    if ( !m_manager->isHostAvailable( id ) )
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
    
    auto remote = m_manager->getRemoteInfo( request->id() );

    if ( remote )
    {
        responseFlag = ( remote->state == RemoteStatus::AWAITING_DUPLEX
            || remote->state == RemoteStatus::ONLINE );
    }

    response->set_response( responseFlag );

    return Status::OK;
}


//
// Server implementation

WarpServer::WarpServer()
    : m_port( WarpServer::DEFAULT_SERVICE_PORT )
    , m_privKey( "" )
    , m_pubKey( "" )
    , m_remoteMgr( nullptr )
    , m_server( nullptr )
{
}

WarpServer::~WarpServer()
{
    if ( m_server )
    {
        stopServer();
    }
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

void WarpServer::setRemoteManager( std::shared_ptr<RemoteManager> mgr )
{
    m_remoteMgr = mgr;
}

std::shared_ptr<RemoteManager> WarpServer::getRemoteManager() const
{
    return m_remoteMgr;
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
        m_port, m_privKey, m_pubKey, m_remoteMgr,
        std::ref( startedPromise ) ) );

    // Wait for the server pointer to become valid
    return startedFuture.get();
}

bool WarpServer::stopServer()
{
    if ( !m_server )
    {
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
    std::shared_ptr<RemoteManager> mgr, std::promise<bool>& startProm )
{
    std::string address = "0.0.0.0:" + std::to_string( port );
    WarpServiceImpl service( mgr );

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
        GRPC_ARG_HTTP2_MIN_SENT_PING_INTERVAL_WITHOUT_DATA_MS, 5 * 1000 );
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
