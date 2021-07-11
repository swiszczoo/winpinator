#include "winpinator_service.hpp"

#include "../zeroconf/mdns_service.hpp"
#include "service_utils.hpp"

#include <SensAPI.h>
#include <iphlpapi.h>
#include <stdlib.h>
#include <winsock2.h>

namespace srv
{

const uint16_t DEFAULT_WINPINATOR_PORT = 52000;
const std::string WinpinatorService::s_warpServiceType
    = "_warpinator._tcp.local.";
const int WinpinatorService::s_pollingIntervalSec = 5;

WinpinatorService::WinpinatorService()
    : m_mutex()
    , m_port( DEFAULT_WINPINATOR_PORT )
    , m_authPort( DEFAULT_WINPINATOR_PORT + 1 )
    , m_ready( false )
    , m_online( false )
    , m_shouldRestart( false )
    , m_stopping( false )
    , m_ip( "" )
    , m_displayName( "" )
{
    DWORD netFlag = NETWORK_ALIVE_LAN;
    bool result = IsNetworkAlive( &netFlag );

    if ( GetLastError() == 0 )
    {
        m_online = result;
    }
    else
    {
        m_online = true;
    }

    m_displayName = Utils::getUserShortName() + '@' + Utils::getHostname();
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

bool WinpinatorService::isOnline() const
{
    return m_online;
}

std::string WinpinatorService::getIpAddress()
{
    std::lock_guard<std::recursive_mutex> guard( m_mutex );

    return m_ip;
}

const std::string& WinpinatorService::getDisplayName() const
{
    return m_displayName;
}

int WinpinatorService::startOnThisThread()
{
    m_stopping = false;

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

    std::mutex varLock;
    std::condition_variable condVar;

    m_pollingThread = std::thread(
        std::bind( &WinpinatorService::networkPollingMain, this,
            std::ref(varLock), std::ref(condVar) ) );

    do
    {
        m_shouldRestart = false;

        std::unique_lock<std::mutex> lock( varLock );
        if ( m_online )
        {
            lock.unlock();

            m_ready = false;
            notifyStateChanged();

            serviceMain();
        }
        else
        {
            condVar.wait( lock );
            m_shouldRestart = true;
        }
    } while ( m_shouldRestart );

    m_stopping = true;

    m_pollingThread.join();

    WSACleanup();

    return EXIT_SUCCESS;
}

void WinpinatorService::serviceMain()
{
    // Reset an event queue
    m_events.Clear();

    // Register 'flush' type service for 3 seconds
    zc::MdnsService flushService( WinpinatorService::s_warpServiceType );
    flushService.setHostname( "Hostname" );
    flushService.setPort( m_port );
    flushService.setTxtRecord( "hostname", Utils::getHostname() );
    flushService.setTxtRecord( "type", "flush" );

    auto ipPairFuture = flushService.registerService();
    zc::MdnsIpPair ipPair = ipPairFuture.get();

    if ( ipPair.valid )
    {
        std::unique_lock<std::recursive_mutex> lock( m_mutex );
        m_ip = ipPair.ipv4;

        notifyIpChanged();
        lock.unlock();
    }

    std::this_thread::sleep_for( std::chrono::seconds( 3 ) );

    flushService.unregisterService();

    // Now register 'real' service
    zc::MdnsService zcService( WinpinatorService::s_warpServiceType );
    zcService.setHostname( "Hostname" );
    zcService.setPort( m_port );
    zcService.setTxtRecord( "hostname", Utils::getHostname() );
    zcService.setTxtRecord( "type", "real" );
    zcService.setTxtRecord( "os", "Windows 10" );
    zcService.setTxtRecord( "api-version", "2" );
    zcService.setTxtRecord( "auth-port", "52001" );

    zcService.registerService();

    // Notify that Winpinator service is now ready
    m_ready = true;
    notifyStateChanged();

    Event ev;
    while ( true )
    {
        m_events.Receive( ev );

        if ( ev.type == EventType::STOP_SERVICE )
        {
            break;
        }
        if ( ev.type == EventType::RESTART_SERVICE )
        {
            m_shouldRestart = true;
            break;
        }
    }
}

void WinpinatorService::notifyStateChanged()
{
    std::lock_guard<std::mutex> guard( m_observersMtx );

    for ( auto& observer : m_observers )
    {
        observer->onStateChanged();
    }
}

void WinpinatorService::notifyIpChanged()
{
    std::lock_guard<std::mutex> guard( m_observersMtx );
    std::string newIp = m_ip;

    for ( auto& observer : m_observers )
    {
        observer->onIpAddressChanged( newIp );
    }
}

int WinpinatorService::networkPollingMain( std::mutex& mtx, 
    std::condition_variable& condVar )
{
    // This thread is responsible for polling
    // if we still have network connection

    DWORD pollingFlag = NETWORK_ALIVE_LAN;

    while ( !m_stopping )
    {
        BOOL result = IsNetworkAlive( &pollingFlag );

        if ( GetLastError() == 0 )
        {
            std::lock_guard<std::mutex> lck( mtx );

            bool oldOnline = m_online;

            if ( result )
            {
                // We are online
                m_online = true;

                if ( !oldOnline )
                {
                    notifyStateChanged();
                }

                condVar.notify_all();
            }
            else 
            {
                // We are offline
                m_online = false;

                if ( oldOnline )
                {
                    notifyStateChanged();

                    std::lock_guard<std::recursive_mutex> guard( m_mutex );
                    m_ip = "";

                    notifyIpChanged();
                }

                Event restartEv;
                restartEv.type = EventType::RESTART_SERVICE;

                m_events.Post( restartEv );
            }
        }

        std::this_thread::sleep_for( std::chrono::seconds( 5 ) );
    }

    return EXIT_SUCCESS;
}

};
