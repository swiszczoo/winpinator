#include "winpinator_service.hpp"

#include "../zeroconf/mdns_client.hpp"
#include "../zeroconf/mdns_service.hpp"

#include "account_picture_extractor.hpp"
#include "auth_manager.hpp"
#include "registration_v1_impl.hpp"
#include "registration_v2_impl.hpp"
#include "service_utils.hpp"
#include "warp_service_impl.hpp"

#include <wx/mstream.h>

#include <algorithm>

#include <SensAPI.h>
#include <iphlpapi.h>
#include <stdlib.h>
#include <winsock2.h>

namespace srv
{

const uint16_t DEFAULT_WINPINATOR_PORT = 52000;
const std::string WinpinatorService::SERVICE_TYPE
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
    , m_error( ServiceError::KEIN_ERROR )
    , m_ip( "" )
    , m_displayName( "" )
    , m_remoteMgr( nullptr )
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

    m_displayName = ( Utils::getUserFullName() + " - "
        + Utils::getUserShortName() + '@' + Utils::getHostname() );

    // Init auth manager with invalid IP (to generate ident)

    zc::MdnsIpPair ipPair;
    ipPair.valid = false;
    AuthManager::get()->update( ipPair, 0 );
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

bool WinpinatorService::hasError() const
{
    return getError() != ServiceError::KEIN_ERROR;
}

ServiceError WinpinatorService::getError() const
{
    return m_error.load();
}

RemoteManager* WinpinatorService::getRemoteManager() const
{
    return m_remoteMgr.get();
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
            std::ref( varLock ), std::ref( condVar ) ) );

    do
    {
        m_shouldRestart = false;

        std::unique_lock<std::mutex> lock( varLock );
        if ( m_online )
        {
            lock.unlock();

            m_ready = false;
            m_error = ServiceError::KEIN_ERROR;
            notifyStateChanged();

            serviceMain();

            if ( m_error != ServiceError::KEIN_ERROR )
            {
                notifyStateChanged();

                // If something went wrong, wait for explicit restart command
                while ( true )
                {
                    Event ev;
                    m_events.Receive( ev );

                    if ( ev.type == EventType::RESTART_SERVICE )
                    {
                        break;
                    }
                }
            }
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

void WinpinatorService::postEvent( const Event& evnt )
{
    m_events.Post( evnt );
}

void WinpinatorService::serviceMain()
{
    // Reset event queue
    m_events.Clear();

    // Initialize remote manager
    m_remoteMgr = std::make_shared<RemoteManager>( this );
    m_remoteMgr->setServiceType( WinpinatorService::SERVICE_TYPE );

    // Try to gather user account picture
    std::string avatarData;
    AccountPictureExtractor extractor;
    if( extractor.process() )
    {
        const wxImage& img = extractor.getHighResImage();
        wxMemoryOutputStream mos;
        img.SaveFile( mos, "image/png" );

        avatarData.resize( mos.GetSize() );
        mos.CopyTo( (void*)avatarData.data(), mos.GetSize() );
    }
    else
    {
        switch ( extractor.getProcessingError() )
        {
        case ExtractorError::AVATAR_FILE_UNREADABLE:
            wxLogDebug( "[APE] ERROR: Avatar file unreadable!" );
            break;
        case ExtractorError::AVATAR_NOT_FOUND:
            wxLogDebug( "[APE] ERROR: Avatar file not found!" );
            break;
        case ExtractorError::REG_KEY_NOT_FOUND:
            wxLogDebug( "[APE] ERROR: Windows Registry key not found!" );
            break;
        }
    }

    // Start the registration service (v1)
    RegistrationV1Server regServer1( "0.0.0.0", m_port );

    // Start the registration service (v2)
    RegistrationV2Server regServer2;
    regServer2.setPort( m_authPort );

    // Start the main RPC service
    WarpServer rpcServer;
    rpcServer.setPort( m_port );
    if ( !avatarData.empty() )
    {
        rpcServer.setAvatarBytes( avatarData );
    }

    // Register 'flush' type service for 3 seconds
    zc::MdnsService flushService( WinpinatorService::SERVICE_TYPE );
    flushService.setHostname( AuthManager::get()->getIdent() );
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

        AuthManager::get()->update( ipPair, m_port );

        // We have an IP so we can start registration servers
        if ( !regServer1.startServer() )
        {
            m_error = ServiceError::REGISTRATION_V1_SERVER_FAILED;
            m_remoteMgr->stop();
            return;
        }

        if( !regServer2.startServer() )
        {
            m_error = ServiceError::REGISTRATION_V2_SERVER_FAILED;
            m_remoteMgr->stop();
            return;
        }

        // ...and the rpc server
        ServerCredentials creds = AuthManager::get()->getServerCreds();
        rpcServer.setPemPrivateKey( creds.privateKey );
        rpcServer.setPemCertificate( creds.publicKey );
        rpcServer.setRemoteManager( m_remoteMgr );
        if( !rpcServer.startServer() )
        {
            m_error = ServiceError::WARP_SERVER_FAILED;
            m_remoteMgr->stop();
            return;
        }
    }
    else 
    {
        m_error = ServiceError::ZEROCONF_SERVER_FAILED;
        m_remoteMgr->stop();
        return;
    }

    std::this_thread::sleep_for( std::chrono::seconds( 3 ) );

    flushService.unregisterService();

    std::this_thread::sleep_for( std::chrono::seconds( 3 ) );

    // Now register 'real' service
    zc::MdnsService zcService( WinpinatorService::SERVICE_TYPE );
    zcService.setHostname( AuthManager::get()->getIdent() );
    zcService.setPort( m_port );
    zcService.setTxtRecord( "hostname", Utils::getHostname() );
    zcService.setTxtRecord( "type", "real" );
    zcService.setTxtRecord( "os", Utils::getOSVersionString() );
    zcService.setTxtRecord( "api-version", "2" );
    zcService.setTxtRecord( "auth-port", std::to_string( m_authPort ) );

    auto secondIpPair = zcService.registerService();
    if ( !secondIpPair.get().valid )
    {
        m_error = ServiceError::ZEROCONF_SERVER_FAILED;
        m_remoteMgr->stop();
        return;
    }

    // Start discovering other hosts on the network
    zc::MdnsClient zcClient( WinpinatorService::SERVICE_TYPE );
    zcClient.setOnAddServiceListener(
        std::bind( &WinpinatorService::onServiceAdded, this,
            std::placeholders::_1 ) );
    zcClient.setOnRemoveServiceListener(
        std::bind( &WinpinatorService::onServiceRemoved, this,
            std::placeholders::_1 ) );
    zcClient.setIgnoredHostname( AuthManager::get()->getIdent() );
    zcClient.startListening();

    // Notify that Winpinator service is now ready
    m_ready = true;
    notifyStateChanged();

    zcClient.repeatQuery();

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
        if ( ev.type == EventType::REPEAT_MDNS_QUERY )
        {
            zcClient.repeatQuery();
        }
        if ( ev.type == EventType::HOST_ADDED )
        {
            m_remoteMgr->processAddHost( *ev.eventData.addedData );
        }
        if ( ev.type == EventType::HOST_REMOVED )
        {
            m_remoteMgr->processRemoveHost( *ev.eventData.removedData );
        }
    }

    // Service cleanup
    zcClient.stopListening();
    m_remoteMgr->stop();

    // Stop registration servers
    regServer1.stopServer();
    regServer2.stopServer();

    // ...and the rpc server
    rpcServer.stopServer();
}

void WinpinatorService::notifyStateChanged()
{
    std::lock_guard<std::recursive_mutex> guard( m_observersMtx );

    for ( auto& observer : m_observers )
    {
        observer->onStateChanged();
    }
}

void WinpinatorService::notifyIpChanged()
{
    std::lock_guard<std::recursive_mutex> guard( m_observersMtx );
    std::string newIp = m_ip;

    for ( auto& observer : m_observers )
    {
        observer->onIpAddressChanged( newIp );
    }
}

void WinpinatorService::onServiceAdded( const zc::MdnsServiceData& serviceData )
{
    wxLogDebug( "Service discovered: %s", wxString( serviceData.name ) );

    // Inform main service thread about new host
    Event ev;
    ev.type = EventType::HOST_ADDED;
    ev.eventData.addedData = std::make_shared<zc::MdnsServiceData>( serviceData );

    postEvent( ev );
}

void WinpinatorService::onServiceRemoved( const std::string& serviceName )
{
    wxLogDebug( "Service removed: %s", wxString( serviceName ) );

    // Inform main service thread about removed host
    Event ev;
    ev.type = EventType::HOST_REMOVED;
    ev.eventData.removedData = std::make_shared<std::string>( serviceName );

    postEvent( ev );
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
