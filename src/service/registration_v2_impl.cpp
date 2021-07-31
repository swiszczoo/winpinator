#include "registration_v2_impl.hpp"

#include "auth_manager.hpp"

#include <grpcpp/grpcpp.h>

#include <algorithm>
#include <cstdlib>

namespace srv
{

Status RegistrationV2Impl::RequestCertificate( ServerContext* context,
    const RegRequest* request, RegResponse* response )
{
    response->set_locked_cert( AuthManager::get()->getEncodedLocalCert() );

    return Status::OK;
}

RegistrationV2Server::RegistrationV2Server()
    : m_port( 52001 )
    , m_server( nullptr )
{
}

RegistrationV2Server::~RegistrationV2Server()
{
    if ( m_server )
    {
        stopServer();
    }
}

void RegistrationV2Server::setPort( uint16_t port )
{
    m_port = port;
}

uint16_t RegistrationV2Server::getPort() const
{
    return m_port;
}

bool RegistrationV2Server::startServer()
{
    if ( m_server )
    {
        // Server is already running
        return false;
    }

    std::promise<bool> startedPromise;
    std::future<bool> startedFuture = startedPromise.get_future();

    m_thread = std::thread( std::bind( &RegistrationV2Server::threadMain, this,
        m_port, std::ref( startedPromise ) ) );

    // Wait for the server pointer to become valid
    return startedFuture.get();
}

bool RegistrationV2Server::stopServer()
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

int RegistrationV2Server::threadMain( uint16_t port, 
    std::promise<bool>& startProm )
{
    std::string address = "0.0.0.0:" + std::to_string( port );
    RegistrationV2Impl service;

    grpc::ServerBuilder builder;
    builder.AddListeningPort( address, grpc::InsecureServerCredentials() );
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
