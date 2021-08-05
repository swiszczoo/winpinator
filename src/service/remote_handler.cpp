#include "remote_handler.hpp"

#include "auth_manager.hpp"
#include "service_utils.hpp"

#include <grpcpp/grpcpp.h>
#include <grpcpp/security/credentials.h>

#include <wx/log.h>

#include <chrono>

namespace srv
{

const int RemoteHandler::CHANNEL_RETRY_WAIT_TIME = 30;
const int RemoteHandler::DUPLEX_MAX_FAILURES = 10;
const int RemoteHandler::DUPLEX_WAIT_PING_TIME = 1;
const int RemoteHandler::CONNECTED_PING_TIME = 20;
const int RemoteHandler::CONNECT_TIMEOUT = 4;
const int RemoteHandler::V2_CHANNEL_RETRY_WAIT_TIME = 10;
const int RemoteHandler::V2_DUPLEX_TIMEOUT = 10;

RemoteHandler::RemoteHandler( RemoteInfoPtr info )
    : m_info( info )
    , m_api( info->apiVersion )
    , m_ident( "" )
    , m_editHandler( []( RemoteInfoPtr dummy ) {} )
{
    m_ident = AuthManager::get()->getIdent();
}

void RemoteHandler::setEditListener(
    std::function<void( RemoteInfoPtr )> listener )
{
    m_editHandler = listener;
}

void RemoteHandler::process()
{
    // First of all, we must ensure that we can establish a stable duplex
    // connection

    if ( m_api == 1 )
    {
        while ( secureLoopV1() ) { }

        std::unique_lock<std::mutex> lock( m_info->mutex );
        setRemoteStatus( lock, RemoteStatus::OFFLINE );
    }
    else if ( m_api == 2 )
    {
        while ( !m_info->stopping )
        {
            secureLoopV2();
        }

        std::unique_lock<std::mutex> lock( m_info->mutex );
        setRemoteStatus( lock, RemoteStatus::OFFLINE );
    }
}

bool RemoteHandler::secureLoopV1()
{
    wxLogDebug( "Remote: Starting a new connection loop for %s (%s:%d)",
        m_info->hostname, m_info->ips.ipv4, m_info->port );

    VoidType dummy;

    std::string cert = AuthManager::get()->getCachedCert(
        m_info->hostname, m_info->ips );

    grpc::SslCredentialsOptions opts;
    opts.pem_root_certs = cert;

    auto creds = grpc::SslCredentials( opts );

    wxString target = wxString::Format( "%s:%d",
        wxString( m_info->ips.ipv4 ), m_info->port );

    gpr_timespec timeout = gpr_time_from_seconds(
        CONNECT_TIMEOUT, GPR_TIMESPAN );

    std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(
        target, creds );

    if ( channel->WaitForConnected( timeout ) )
    {
        std::unique_lock<std::mutex> lock( m_info->mutex );
        m_info->stub = Warp::NewStub( channel );
        lock.unlock();

        int duplexFailCounter = 0;
        bool onePing = false;

        while ( !m_info->stopping )
        {
            if ( m_info->busy )
            {
                wxLogDebug( "Remote Ping: Skipping keepalive ping to "
                            "%s (%s:%d) (busy)",
                    m_info->hostname, m_info->ips.ipv4, m_info->port );

                m_info->busy = false;
            }
            else
            {
                wxLogDebug( "Remote Ping: to   %s (%s:%d)",
                    m_info->hostname, m_info->ips.ipv4, m_info->port );

                LookupName pingData;
                pingData.set_id( m_ident );
                pingData.set_readable_name( Utils::getHostname() );

                grpc::ClientContext ctx;
                setTimeout( ctx, 5 );

                if ( !m_info->stub->Ping( &ctx, pingData, &dummy ).ok() )
                {
                    wxLogDebug( "Remote: Ping failed, shutting down %s (%s:%d)",
                        m_info->hostname, m_info->ips.ipv4, m_info->port );

                    break;
                }

                if ( !onePing )
                {
                    setRemoteStatus( lock, RemoteStatus::AWAITING_DUPLEX );

                    if ( checkDuplexConnection() )
                    {
                        wxLogDebug( "Remote: Connected to %s (%s:%d)",
                            m_info->hostname, m_info->ips.ipv4, m_info->port );

                        setRemoteStatus( lock, RemoteStatus::ONLINE );

                        updateRemoteMachineInfo();
                        updateRemoteMachineAvatar();

                        onePing = true;
                    }
                    else
                    {
                        duplexFailCounter++;
                        if ( duplexFailCounter > DUPLEX_MAX_FAILURES )
                        {
                            wxLogDebug( "Remote: CheckDuplexConnection to %s"
                                        " (%s:%d) failed too many times",
                                m_info->hostname, m_info->ips.ipv4,
                                m_info->port );

                            lock.lock();

                            if ( m_info->stopping )
                            {
                                return false;
                            }

                            m_info->stopVar.wait_for( lock,
                                std::chrono::seconds( CHANNEL_RETRY_WAIT_TIME ) );
                            return true;
                        }
                    }
                }
            }

            lock.lock();

            if ( m_info->stopping )
            {
                return false;
            }

            int waitSecs = ( m_info->state == RemoteStatus::ONLINE )
                ? CONNECTED_PING_TIME
                : DUPLEX_WAIT_PING_TIME;

            m_info->stopVar.wait_for( lock,
                std::chrono::seconds( waitSecs ) );
            lock.unlock();
        }

        if ( m_info->hasZcPresence && !m_info->stopping )
        {
            return true;
        }

        return false;
    }
    else
    {
        std::unique_lock<std::mutex> lock( m_info->mutex );
        setRemoteStatus( lock, RemoteStatus::UNREACHABLE );

        if ( m_info->stopping )
        {
            return false;
        }

        wxLogDebug( "Remote: Unable to establish secure connection with "
                    "%s (%s:%d). Trying again in %ds",
            wxString( m_info->hostname ),
            m_info->ips.ipv4, m_info->port, CHANNEL_RETRY_WAIT_TIME );

        m_info->stopVar.wait_for( lock,
            std::chrono::seconds( CHANNEL_RETRY_WAIT_TIME ) );
        return true;
    }

    return true;
}

bool RemoteHandler::secureLoopV2()
{
    wxLogDebug( "Remote: Starting a new connection loop for %s (%s:%d)"
                " - api version 2",
        m_info->hostname, m_info->ips.ipv4, m_info->port );

    std::string cert = AuthManager::get()->getCachedCert(
        m_info->hostname, m_info->ips );

    grpc::SslCredentialsOptions opts;
    opts.pem_root_certs = cert;

    auto creds = grpc::SslCredentials( opts );

    wxString target = wxString::Format( "%s:%d",
        wxString( m_info->ips.ipv4 ), m_info->port );

    gpr_timespec timeout = gpr_time_from_seconds(
        CONNECT_TIMEOUT, GPR_TIMESPAN );

    auto maxDline = std::chrono::time_point<std::chrono::system_clock>::max();

    std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(
        target, creds );

    if ( channel->WaitForConnected( timeout ) )
    {
        grpc::CompletionQueue cq;
        channel->NotifyOnStateChange( channel->GetState( false ), 
            maxDline, &cq, nullptr );

        // Start a thread that listens to disconnects
        std::thread discoListener( [this, &cq, channel, maxDline]() {
            while ( channel->GetState( false ) == GRPC_CHANNEL_READY )
            {
                void* tag;
                bool ok;
                if ( cq.Next( &tag, &ok ) )
                {

                    if ( channel->GetState( false ) != GRPC_CHANNEL_READY )
                    {
                        break;
                    }

                    channel->NotifyOnStateChange( channel->GetState( false ),
                        maxDline, &cq, nullptr );
                }
            }

            // Unlock thread
            m_info->stopping = true;
            m_info->stopVar.notify_all();
        } );

        std::unique_lock<std::mutex> lock( m_info->mutex );
        m_info->stub = Warp::NewStub( channel );
        setRemoteStatus( lock, RemoteStatus::AWAITING_DUPLEX );
        lock.unlock();

        std::future<bool> duplex = waitForDuplex( V2_DUPLEX_TIMEOUT );
        if ( !duplex.get() )
        {
            std::unique_lock<std::mutex> lock( m_info->mutex );
            setRemoteStatus( lock, RemoteStatus::UNREACHABLE );

            if ( m_info->stopping )
            {
                return false;
            }

            wxLogDebug( "Problem while awaiting duplex response"
                        "- api version 2: %s",
                wxString( m_info->hostname ) );

            wxLogDebug( "Retrying in 10s..." );

            m_info->stopVar.wait_for( lock,
                std::chrono::seconds( V2_CHANNEL_RETRY_WAIT_TIME ) );
            cq.Shutdown();
            discoListener.join();
            return true;
        }

        setRemoteStatus( lock, RemoteStatus::ONLINE );

        updateRemoteMachineInfo();
        updateRemoteMachineAvatar();

        lock.lock();

        if ( m_info->stopping )
        {
            cq.Shutdown();
            discoListener.join();
            return false;
        }

        m_info->stopVar.wait( lock );
        cq.Shutdown();
        discoListener.join();
    }
    else
    {
        std::unique_lock<std::mutex> lock( m_info->mutex );
        setRemoteStatus( lock, RemoteStatus::UNREACHABLE );

        if ( m_info->stopping )
        {
            return false;
        }

        wxLogDebug( "Problem while waiting for channel - api version 2: %s",
            wxString( m_info->hostname ) );

        wxLogDebug( "Retrying in 10s..." );

        m_info->stopVar.wait_for( lock,
            std::chrono::seconds( V2_CHANNEL_RETRY_WAIT_TIME ) );
        return true;
    }

    return true;
}

bool RemoteHandler::checkDuplexConnection()
{
    wxLogDebug( "Remote: checking duplex with '%s'", m_info->hostname );

    grpc::ClientContext ctx;
    ctx.set_deadline( std::chrono::system_clock::now()
        + std::chrono::seconds( 3 ) );

    LookupName request;
    HaveDuplex response;

    request.set_id( m_ident );
    request.set_readable_name( Utils::getHostname() );

    if ( m_info->stub->CheckDuplexConnection( &ctx, request, &response ).ok() )
    {
        return response.response();
    }

    return false;
}

void RemoteHandler::updateRemoteMachineInfo()
{
    wxLogDebug( "Remote RPC: calling GetRemoteMachineInfo on '%s'",
        m_info->hostname );

    std::shared_ptr<grpc::ClientContext> ctx
        = std::make_shared<grpc::ClientContext>();

    std::shared_ptr<LookupName> request = std::make_shared<LookupName>();
    std::shared_ptr<RemoteMachineInfo> response
        = std::make_shared<RemoteMachineInfo>();

    request->set_id( m_ident );
    request->set_readable_name( Utils::getHostname() );

    m_info->stub->experimental_async()->GetRemoteMachineInfo( ctx.get(),
        request.get(), response.get(),
        [this, ctx, request, response]( grpc::Status status ) {
            if ( status.ok() )
            {
                std::unique_lock<std::mutex> lock( m_info->mutex );

                wxString dispName = wxString::FromUTF8(
                    response->display_name() );

                m_info->fullName = dispName.ToStdWstring();
                m_info->shortName = response->user_name();
                setRemoteStatus( lock, RemoteStatus::ONLINE );
            }
        } );
}

void RemoteHandler::updateRemoteMachineAvatar()
{
    wxLogDebug( "Remote RPC: calling GetRemoteMachineAvatar on '%s'",
        m_info->hostname );

    std::shared_ptr<grpc::ClientContext> ctx
        = std::make_shared<grpc::ClientContext>();

    std::shared_ptr<LookupName> request = std::make_shared<LookupName>();

    class Reader : public grpc::experimental::ClientReadReactor<RemoteMachineAvatar>
    {
    public:
        Reader( RemoteInfoPtr info, std::function<void( RemoteInfoPtr )> edit )
            : m_inst( nullptr )
            , m_info( info )
            , m_editHandler( edit )
            , m_buffer( "" )
        {
        }

        void start()
        {
            StartRead( &m_chunk );
            StartCall();
        }

        void OnReadDone( bool ok ) override
        {
            if ( ok )
            {
                m_buffer += m_chunk.avatar_chunk();
            }
        }

        void OnDone( const grpc::Status& s ) override
        {
            if ( s.ok() )
            {
                std::lock_guard<std::mutex> lock( m_info->mutex );
                m_info->avatarBuffer = m_buffer;
                m_editHandler( m_info );
            }

            // Release ref to instance
            m_inst = nullptr;
        }

        void setInstance( std::shared_ptr<Reader> inst )
        {
            m_inst = inst;
        }

    private:
        // "Manual" closure variables
        std::shared_ptr<Reader> m_inst;
        RemoteInfoPtr m_info;
        std::function<void( RemoteInfoPtr )> m_editHandler;

        std::string m_buffer;
        RemoteMachineAvatar m_chunk;
    };

    std::shared_ptr<Reader> responseReader = std::make_shared<Reader>(
        m_info, m_editHandler );
    responseReader->setInstance( responseReader );

    m_info->stub->experimental_async()->GetRemoteMachineAvatar( ctx.get(),
        request.get(), responseReader.get() );

    responseReader->start();
}

std::future<bool> RemoteHandler::waitForDuplex( int timeout )
{
    std::shared_ptr<grpc::ClientContext> ctx
        = std::make_shared<grpc::ClientContext>();

    std::shared_ptr<LookupName> request = std::make_shared<LookupName>();
    std::shared_ptr<HaveDuplex> response
        = std::make_shared<HaveDuplex>();

    request->set_id( m_ident );
    request->set_readable_name( Utils::getHostname() );

    setTimeout( *ctx, timeout );

    std::shared_ptr<std::promise<bool>> result
        = std::make_shared<std::promise<bool>>();

    m_info->stub->experimental_async()->WaitingForDuplex( ctx.get(),
        request.get(), response.get(),
        [this, ctx, request, response, result]( grpc::Status status ) {
            result->set_value( status.ok() );
        } );

    return result->get_future();
}

void RemoteHandler::setTimeout( grpc::ClientContext& context, int seconds )
{
    auto timePoint = std::chrono::system_clock::now()
        + std::chrono::seconds( seconds );
    context.set_deadline( timePoint );
}

void RemoteHandler::setRemoteStatus( std::unique_lock<std::mutex>& lck,
    RemoteStatus newStatus )
{
    bool hasLock = lck.owns_lock();

    if ( !hasLock )
    {
        lck.lock();
    }

    m_info->state = newStatus;
    m_editHandler( m_info );

    if ( !hasLock )
    {
        lck.unlock();
    }
}

};
