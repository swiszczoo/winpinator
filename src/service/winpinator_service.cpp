#include "winpinator_service.hpp"

#include "../zeroconf/mdns_service.hpp"

#include <iphlpapi.h>
#include <stdlib.h>
#include <winsock2.h>

namespace srv
{

const uint16_t DEFAULT_WINPINATOR_PORT = 52000;
const std::string WinpinatorService::s_warpServiceType
    = "_warpinator._tcp.local.";

WinpinatorService::WinpinatorService()
    : m_mutex()
    , m_port( DEFAULT_WINPINATOR_PORT )
    , m_authPort( DEFAULT_WINPINATOR_PORT + 1 )
    , m_ready( false )
{
}

void WinpinatorService::setGrpcPort( uint16_t port )
{
    m_port = port;
}

uint16_t WinpinatorService::getGrpcPort() const
{
    return m_port;
}

void WinpinatorService::setAuthPort( uint16_t authPort )
{
    m_authPort = authPort;
}

uint16_t WinpinatorService::getAuthPort() const
{
    return m_authPort;
}

bool WinpinatorService::isServiceReady() const
{
    return m_ready;
}

int WinpinatorService::startOnThisThread()
{
    // Init WinSock library
    {
        WORD versionWanted = MAKEWORD( 1, 1 );
        WSADATA wsaData;
        if ( WSAStartup( versionWanted, &wsaData ) )
        {
            printf( "Failed to initialize WinSock!\n" );
            return -1;
        }
    }

    // Register 'flush' type service for 3 seconds
    zc::MdnsService flushService( WinpinatorService::s_warpServiceType );
    flushService.setHostname( "Hostname" );
    flushService.setPort( m_port );
    flushService.setTxtRecord( "hostname", "DESK-X570" );
    flushService.setTxtRecord( "type", "flush" );

    flushService.registerService();

    std::this_thread::sleep_for( std::chrono::seconds( 3 ) );

    flushService.unregisterService();

    // Now register 'real' service
    zc::MdnsService zcService( WinpinatorService::s_warpServiceType );
    zcService.setHostname( "Hostname" );
    zcService.setPort( m_port );
    zcService.setTxtRecord( "hostname", "DESK-X570" );
    zcService.setTxtRecord( "type", "real" );
    zcService.setTxtRecord( "os", "Windows 10" );
    zcService.setTxtRecord( "api-version", "2" );
    zcService.setTxtRecord( "auth-port", "52001" );

    zcService.registerService();

    // Notify that Winpinator service is now ready
    m_ready = true;
    notifyStateChanged();

    while ( true )
    {
        std::this_thread::sleep_for( std::chrono::seconds( 10 ) );
    }

    WSACleanup();

    return EXIT_SUCCESS;
}

void WinpinatorService::notifyStateChanged()
{
    std::lock_guard<std::mutex> guard( m_observersMtx );

    for ( auto& observer : m_observers )
    {
        observer->onStateChanged();
    }
}

};
