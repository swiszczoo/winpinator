#include "remote_manager.hpp"

#include "auth_manager.hpp"
#include "remote_handler.hpp"
#include "service_utils.hpp"

#include "../proto-gen/warp.grpc.pb.h"

#include <wx/log.h>
#include <wx/socket.h>
#include <wx/wx.h>

#include <grpcpp/create_channel.h>
#include <grpcpp/grpcpp.h>

#include <chrono>

#include <stdlib.h>

namespace srv
{

const std::string RemoteManager::FALLBACK_OS = "Linux";
const std::string RemoteManager::REQUEST = "REQUEST";

RemoteManager::RemoteManager( ObservableService* service )
    : m_srv( service )
    , m_srvType( "" )
{
}

RemoteManager::~RemoteManager()
{
    stop();
}

void RemoteManager::stop()
{
    std::lock_guard<std::mutex> guard( m_mutex );

    for ( std::shared_ptr<RemoteInfo>& info : m_hosts )
    {
        std::lock_guard<std::mutex> lock( info->mutex );

        info->stopping = true;
        info->stopVar.notify_all();
    }

    for ( std::shared_ptr<RemoteInfo>& info : m_hosts )
    {
        if ( info->thread.joinable() )
        {
            info->thread.join();
        }
    }

    m_hosts.clear();
}

size_t RemoteManager::getTotalHostsCount()
{
    std::lock_guard<std::mutex> guard( m_mutex );

    return m_hosts.size();
}

size_t RemoteManager::getVisibleHostsCount()
{
    std::lock_guard<std::mutex> guard( m_mutex );
    return _getVisibleHostsCount();
}

std::vector<RemoteInfoPtr> RemoteManager::generateCurrentHostList()
{
    std::lock_guard<std::mutex> guard( m_mutex );

    std::vector<RemoteInfoPtr> result;

    for ( const std::shared_ptr<RemoteInfo>& info : m_hosts )
    {
        if ( info->visible )
        {
            result.push_back( info );
        }
    }

    return result;
}

void RemoteManager::setServiceType( const std::string& serviceType )
{
    std::lock_guard<std::mutex> guard( m_mutex );

    m_srvType = serviceType;

    if ( m_srvType[m_srvType.length() - 1] != '.' )
    {
        m_srvType += '.';
    }
}

const std::string& RemoteManager::getServiceType()
{
    std::lock_guard<std::mutex> guard( m_mutex );

    return m_srvType;
}

void RemoteManager::processAddHost( const zc::MdnsServiceData& data )
{
    std::lock_guard<std::mutex> guard( m_mutex );

    // Check if our peer is a 'real' type host
    // Otherwise ignore it
    if ( data.txtRecords.find( "type" ) == data.txtRecords.end() )
    {
        return;
    }

    if ( data.txtRecords.find( "hostname" ) == data.txtRecords.end() )
    {
        return;
    }

    if ( data.txtRecords.at( "type" ) != "real" )
    {
        return;
    }

    // Fill RemoteInfo struct
    std::shared_ptr<RemoteInfo> info = std::make_shared<RemoteInfo>();
    info->visible = false;
    info->id = stripServiceFromIdent( data.name );
    info->ips.valid = true;
    info->ips.ipv4 = data.ipv4;
    info->ips.ipv6 = data.ipv6;
    info->port = data.port;
    info->hostname = data.txtRecords.at( "hostname" );
    info->state = RemoteStatus::REGISTRATION;
    info->busy = false;
    info->hasZcPresence = true;
    info->stopping = ATOMIC_VAR_INIT( false );

    auto pair = std::make_pair( info->hostname, info->ips );
    if ( m_hostSet.find( pair ) == m_hostSet.end() )
    {
        m_hostSet.insert( pair );
    }
    else
    {
        // This host already exists, try to find it and unlock current wait op,
        // then return

        for ( std::shared_ptr<RemoteInfo>& hostInfo : m_hosts )
        {
            if ( info->hostname == hostInfo->hostname
                && info->ips == hostInfo->ips )
            {
                hostInfo->stopVar.notify_all();
                break;
            }
        }

        return;
    }

    if ( data.txtRecords.find( "api-version" ) == data.txtRecords.end() )
    {
        info->apiVersion = 1;
    }
    else
    {
        info->apiVersion = atoi( data.txtRecords.at( "api-version" ).c_str() );

        if ( info->apiVersion < 1 )
        {
            info->apiVersion = 1;
        }
        if ( info->apiVersion > 2 )
        {
            info->apiVersion = 2;
        }
    }

    if ( data.txtRecords.find( "auth-port" ) == data.txtRecords.end() )
    {
        info->authPort = info->port;
    }
    else
    {
        info->authPort = (uint16_t)atoi(
            data.txtRecords.at( "auth-port" ).c_str() );
    }

    if ( data.txtRecords.find( "os" ) == data.txtRecords.end() )
    {
        info->os = RemoteManager::FALLBACK_OS;
    }
    else
    {
        info->os = data.txtRecords.at( "os" );
    }

    // Start remote handler thread
    info->thread = std::thread( std::bind(
        &RemoteManager::remoteThreadEntry, this, info ) );

    m_hosts.push_back( info );
}

void RemoteManager::processRemoveHost( const std::string& id )
{
    std::lock_guard<std::mutex> guard( m_mutex );

    std::string strippedId = stripServiceFromIdent( id );

    for ( std::shared_ptr<RemoteInfo>& info : m_hosts )
    {
        if ( info->id == strippedId )
        {
            info->hasZcPresence = false;
        }
    }
}

bool RemoteManager::isHostAvailable( const std::string& id, bool strip )
{
    std::lock_guard<std::mutex> guard( m_mutex );

    if ( strip )
    {
        std::string strippedId = stripServiceFromIdent( id );

        for ( std::shared_ptr<RemoteInfo>& hostInfo : m_hosts )
        {
            if ( hostInfo->id == strippedId )
            {
                return true;
            }
        }
    }
    else 
    {
        for ( std::shared_ptr<RemoteInfo>& hostInfo : m_hosts )
        {
            if ( hostInfo->id == id )
            {
                return true;
            }
        }
    }

    return false;
}

std::shared_ptr<RemoteInfo>
RemoteManager::getRemoteInfo( const std::string& id, bool strip )
{
    std::lock_guard<std::mutex> guard( m_mutex );

    if ( strip )
    {
        std::string strippedId = stripServiceFromIdent( id );

        for ( std::shared_ptr<RemoteInfo>& hostInfo : m_hosts )
        {
            if ( hostInfo->id == strippedId )
            {
                return hostInfo;
            }
        }
    }
    else
    {
        for ( std::shared_ptr<RemoteInfo>& hostInfo : m_hosts )
        {
            if ( hostInfo->id == id )
            {
                return hostInfo;
            }
        }
    }

    return nullptr;
}

std::string RemoteManager::stripServiceFromIdent(
    const std::string& identStr ) const
{
    if ( identStr.length() <= m_srvType.length() )
    {
        return identStr;
    }

    size_t subOffset = identStr.length() - m_srvType.length();

    if ( identStr.substr( subOffset ) == m_srvType 
        && identStr[subOffset - 1] == '.' )
    {
        return identStr.substr( 0, subOffset - 1 );
    }

    return identStr;
}

int RemoteManager::remoteThreadEntry( std::shared_ptr<RemoteInfo> serviceInfo )
{
    if ( serviceInfo->apiVersion == 1 )
    {
        if ( !doRegistrationV1( serviceInfo.get() ) )
        {
            wxLogDebug( "Unable to register with %s (%s:%d) - api version 1",
                serviceInfo->hostname, serviceInfo->ips.ipv4,
                serviceInfo->port );

            return EXIT_SUCCESS;
        }
    }
    else if ( serviceInfo->apiVersion == 2 )
    {
        if ( !doRegistrationV2( serviceInfo.get() ) )
        {
            wxLogDebug( "Unable to register with %s (%s:%d) - api version 2",
                serviceInfo->hostname, serviceInfo->ips.ipv4,
                serviceInfo->authPort );

            return EXIT_SUCCESS;
        }
    }

    if ( serviceInfo->stopping )
    {
        return EXIT_SUCCESS;
    }

    // Notify about new host
    std::unique_lock<std::mutex> lock( m_mutex );

    serviceInfo->visible = true;
    serviceInfo->state = RemoteStatus::INIT_CONNECTING;

    size_t hostCount = _getVisibleHostsCount();

    m_srv->notifyObservers(
        [hostCount, serviceInfo]( IServiceObserver* observer ) {
            observer->onAddHost( serviceInfo );
            observer->onHostCountChanged( hostCount );
        } );
    lock.unlock();

    // We are now successfully registered
    // Pass control over our remote to RemoteHandler

    RemoteHandler handler( serviceInfo );
    handler.setEditListener( [this]( RemoteInfoPtr info ) {
        std::lock_guard<std::mutex> lock( m_mutex );
        m_srv->notifyObservers( [info]( IServiceObserver* observer ) {
            observer->onEditHost( info );
        } );
    } );
    handler.process();

    return EXIT_SUCCESS;
}

bool RemoteManager::doRegistrationV1( RemoteInfo* serviceInfo )
{
    wxLogDebug( "Registering with %s (%s:%d) - api version 1",
        serviceInfo->hostname, serviceInfo->ips.ipv4, serviceInfo->port );

    do
    {
        wxLogDebug( "Requesting cert from %s...", serviceInfo->hostname );
        int tryCount = 0;

        wxIPV4address address;
        address.AnyAddress();

        wxIPV4address destination;
        destination.Hostname( serviceInfo->ips.ipv4 );
        destination.Service( serviceInfo->port );

        while ( tryCount < 3 )
        {
            wxDatagramSocket sock( address, wxSOCKET_BLOCK );
            sock.SetTimeout( 1 );
            sock.SendTo( destination, RemoteManager::REQUEST.data(),
                RemoteManager::REQUEST.size() );

            wxIPV4address replyAddr;
            char replyBuf[1500];

            sock.RecvFrom( replyAddr, replyBuf, 1500 );
            size_t length = sock.LastReadCount();

            if ( length == 0 ) // Timeout
            {
                tryCount++;
                continue;
            }

            if ( replyAddr == destination )
            {
                wxLogDebug( "Got remote cert from %s", serviceInfo->hostname );

                std::string msg( replyBuf, length );

                return AuthManager::get()->processRemoteCert(
                    serviceInfo->hostname, serviceInfo->ips, msg );
            }
        }

        wxLogDebug( "Can't get cert from %s. Retry limit (3) exceeded. "
                    "Waiting 30s.",
            serviceInfo->hostname );

        std::unique_lock<std::mutex> lock( serviceInfo->mutex );
        if ( serviceInfo->stopping )
        {
            return false;
        }
        serviceInfo->stopVar.wait_for( lock, std::chrono::seconds( 30 ) );

    } while ( !serviceInfo->stopping );

    return false;
}

bool RemoteManager::doRegistrationV2( RemoteInfo* serviceInfo )
{
    wxLogDebug( "Registering with %s (%s:%d) - api version 2",
        serviceInfo->id, serviceInfo->ips.ipv4, serviceInfo->authPort );

    wxString target = wxString::Format( "%s:%d",
        wxString( serviceInfo->ips.ipv4 ), serviceInfo->authPort );

    gpr_timespec timeout = gpr_time_from_seconds( 5, GPR_TIMESPAN );

    do
    {
        wxLogDebug( "Requesting cert from %s...", serviceInfo->hostname );

        std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(
            target.ToStdString(), grpc::InsecureChannelCredentials() );

        if ( channel->WaitForConnected( timeout ) )
        {
            auto stub = WarpRegistration::NewStub( channel );

            RegRequest request;
            request.set_ip( serviceInfo->ips.ipv4 );
            request.set_hostname( Utils::getHostname() );

            RegResponse response;

            grpc::ClientContext context;

            grpc::Status status = stub->RequestCertificate(
                &context, request, &response );

            if ( status.ok() )
            {
                wxLogDebug( "Got remote cert from %s", serviceInfo->hostname );

                const std::string& lockedCert = response.locked_cert();

                return AuthManager::get()->processRemoteCert(
                    serviceInfo->hostname, serviceInfo->ips, lockedCert );
            }
        }

        wxLogDebug( "Can't get cert from %s. Waiting 10s.",
            serviceInfo->hostname );

        std::unique_lock<std::mutex> lock( serviceInfo->mutex );
        if ( serviceInfo->stopping )
        {
            return false;
        }
        serviceInfo->stopVar.wait_for( lock, std::chrono::seconds( 10 ) );

    } while ( !serviceInfo->stopping );

    return false;
}

size_t RemoteManager::_getVisibleHostsCount()
{
    size_t count = 0;

    for ( const std::shared_ptr<RemoteInfo>& info : m_hosts )
    {
        if ( info->visible )
        {
            count++;
        }
    }

    return count;
}

};
