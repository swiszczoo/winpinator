#include "registration_v1_impl.hpp"

#include "../thread_name.hpp"
#include "auth_manager.hpp"

#include <stdlib.h>

namespace srv
{

const std::string RegistrationV1Server::REQUEST = "REQUEST";

RegistrationV1Server::RegistrationV1Server( std::string address, uint16_t port )
    : m_sock( nullptr )
    , m_ipAddr( address )
    , m_port( port )
    , m_exiting( false )
{
    wxLogNull logNull;

    wxIPV4address addr;
    if ( !addr.Hostname( address ) )
    {
        addr.AnyAddress();
    }

    addr.Service( port );

    m_sock = std::make_unique<wxDatagramSocket>( addr, wxSOCKET_BLOCK );
}

RegistrationV1Server::~RegistrationV1Server()
{
    stopServer();
}

bool RegistrationV1Server::startServer()
{
    if ( m_thread.joinable() )
    {
        return false;
    }

    if ( !m_sock->IsOk() )
    {
        wxLogDebug( "Registration v1 socket is broken!" );
        return false;
    }

    m_exiting = false;
    m_thread = std::thread( std::bind(
        &RegistrationV1Server::threadMain, this ) );

    return true;
}

bool RegistrationV1Server::stopServer()
{
    if ( !m_thread.joinable() )
    {
        return false;
    }

    m_exiting = true;
    m_sock->Close();
    m_sock->InterruptWait();

    m_thread.join();
    
    return true;
}

int RegistrationV1Server::threadMain()
{
    setThreadName( "Registration V1 worker" );

    while ( !m_exiting )
    {
        wxIPV4address peerAddr;

        // Default MTU is 1500 bytes, so it should be enough
        char buf[1500];
        size_t n;

        m_sock->RecvFrom( peerAddr, buf, 1500 );
        n = m_sock->LastReadCount();

        if ( m_exiting )
        {
            break;
        }

        std::string data( buf, n );
        if ( data == RegistrationV1Server::REQUEST )
        {
            // Respond with the actual public PEM cert
            std::string certData = AuthManager::get()->getEncodedLocalCert();
            m_sock->SendTo( peerAddr, certData.data(), certData.length() );
            
        }
    }

    return EXIT_SUCCESS;
}

};
