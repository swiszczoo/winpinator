#include "globals.hpp"
#include "gui/winpinator_frame.hpp"
#include "running_instance_detector.hpp"
#include "service/service_observer.hpp"
#include "service/winpinator_service.hpp"
#include "thread_name.hpp"
#include "tray_icon.hpp"
#include "winpinator_dde_client.hpp"
#include "winpinator_dde_server.hpp"

#include <google/protobuf/message_lite.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <wx/log.h>
#include <wx/socket.h>
#include <wx/wx.h>

#include <Windows.h>
#include <stdlib.h>

#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <thread>

int serviceMain( std::promise<void>& promise );

class WinpinatorApp : public wxApp, srv::IServiceObserver
{
public:
    WinpinatorApp();
    virtual bool OnInit() override;
    virtual int OnExit() override;

private:
    enum class EventType
    {
        STATE_CHANGED,
        OPEN_TRANSFER_UI
    };

    wxLocale m_locale;
    gui::WinpinatorFrame* m_topLvl;
    TrayIcon* m_trayIcon;
    std::thread m_srvThread;

    std::shared_ptr<RunningInstanceDetector> m_detector;
    std::unique_ptr<WinpinatorDDEServer> m_ddeServer;

    void showMainFrame();
    void onMainFrameDestroyed( wxWindowDestroyEvent& event );
    void onDDEOpenCalled( wxCommandEvent& event );
    void onRestore( wxCommandEvent& event );
    void onServiceEvent( wxThreadEvent& event );
    void onExitApp( wxCommandEvent& event );

    // Service observer methods:
    virtual void onStateChanged();
    virtual void onOpenTransferUI( wxString remoteId );
};

wxIMPLEMENT_APP( WinpinatorApp );

WinpinatorApp::WinpinatorApp()
    : m_locale( wxLANGUAGE_ENGLISH_US )
    , m_topLvl( nullptr )
    , m_trayIcon( nullptr )
    , m_detector( nullptr )
    , m_ddeServer( nullptr )
{
    // Events

    Bind( EVT_OPEN_APP_WINDOW, &WinpinatorApp::onDDEOpenCalled, this );
    Bind( EVT_RESTORE_WINDOW, &WinpinatorApp::onRestore, this );
    Bind( wxEVT_THREAD, &WinpinatorApp::onServiceEvent, this );
    Bind( EVT_EXIT_APP, &WinpinatorApp::onExitApp, this );
}

bool WinpinatorApp::OnInit()
{
    wxInitAllImageHandlers();
    wxSocketBase::Initialize();

    // Start running instance detector
    m_detector = std::make_unique<RunningInstanceDetector>( "winpinator.lock" );

    if ( m_detector->isAnotherInstanceRunning() )
    {
        wxLogDebug( "[RUNINST] Another instance is detected!" );

        WinpinatorDDEClient client;
        if ( client.isConnected() )
        {
            client.execOpen();
        }
        else
        {
            wxLogDebug( "Something went wrong. Can't connect to DDE server!" );
        }

        // Do not start another instance
        return false;
    }
    else
    {
        wxLogDebug( "[RUNINST] First instance." );

        // Start the DDE server
        m_ddeServer = std::make_unique<WinpinatorDDEServer>( this );
    }


    // Start the service in the background thread

    // We must ensure that ui thread waits until global service pointer
    // becomes valid. This can be accomplished with std::promise and
    // std::future

    std::promise<void> promise;
    std::future<void> future = promise.get_future();

    m_srvThread = std::thread( std::bind( serviceMain,
        std::ref( promise ) ) );

    // Wait for the service to become valid
    future.wait();

    // Set up tray icon
    m_trayIcon = new TrayIcon();
    m_trayIcon->setEventHandler( this );

    auto serv = Globals::get()->getWinpinatorServiceInstance();
    observeService( serv );

    if ( serv->isServiceReady() )
    {
        m_trayIcon->setOkState();
    }

    // Show main window
    showMainFrame();

    return true;
}

int WinpinatorApp::OnExit()
{
    // Unregister process
    m_detector = nullptr;
    m_ddeServer = nullptr;

    // Wait for the background thread to finish before exiting the process
    m_srvThread.join();

    // Shut down protobuf
    google::protobuf::ShutdownProtobufLibrary();

    return EXIT_SUCCESS;
}

void WinpinatorApp::showMainFrame()
{
    if ( m_topLvl )
    {
        return;
    }

    m_topLvl = new gui::WinpinatorFrame( nullptr );
    m_topLvl->Show( true );

    m_topLvl->Bind( wxEVT_DESTROY, &WinpinatorApp::onMainFrameDestroyed, this );
}

void WinpinatorApp::onMainFrameDestroyed( wxWindowDestroyEvent& event )
{
    if ( event.GetEventObject() == m_topLvl )
    {
        m_topLvl->Unbind(
            wxEVT_DESTROY, &WinpinatorApp::onMainFrameDestroyed, this );

        m_topLvl = nullptr;
    }
}

void WinpinatorApp::onDDEOpenCalled( wxCommandEvent& event )
{
    if ( !m_topLvl )
    {
        showMainFrame();
    }
}

void WinpinatorApp::onRestore( wxCommandEvent& event )
{
    if ( !m_topLvl )
    {
        showMainFrame();
    }
}

void WinpinatorApp::onServiceEvent( wxThreadEvent& event )
{
    auto serv = Globals::get()->getWinpinatorServiceInstance();

    if ( event.GetInt() == (int)EventType::STATE_CHANGED )
    {
        if ( serv->hasError() && !m_trayIcon->isInErrorState() )
        {
            m_trayIcon->setErrorState();
            return;
        }

        if ( !serv->isOnline() && !m_trayIcon->isInErrorState() )
        {
            m_trayIcon->setErrorState();
        }
        else
        {
            if ( serv->isServiceReady() && !m_trayIcon->isInOkState() )
            {
                m_trayIcon->setOkState();
            }
            else if ( !serv->isServiceReady() && !m_trayIcon->isInWaitState() )
            {
                m_trayIcon->setWaitState();
            }
        }
    }
    else if ( event.GetInt() == (int)EventType::OPEN_TRANSFER_UI )
    {
        showMainFrame();

        m_topLvl->showTransferScreen( event.GetString() );
    }
}

void WinpinatorApp::onExitApp( wxCommandEvent& event )
{
    if ( m_topLvl )
    {
        m_topLvl->Close( true );
    }

    srv::Event evnt;
    evnt.type = srv::EventType::STOP_SERVICE;
    
    auto serv = Globals::get()->getWinpinatorServiceInstance();
    if ( serv )
    {
        serv->postEvent( evnt );
    }

    m_trayIcon->Destroy();
    m_trayIcon = nullptr;
}

void WinpinatorApp::onStateChanged()
{
    wxThreadEvent evnt( wxEVT_THREAD );
    evnt.SetInt( (int)EventType::STATE_CHANGED );
    wxQueueEvent( this, evnt.Clone() );
}

void WinpinatorApp::onOpenTransferUI( wxString remoteId )
{
    wxThreadEvent evnt( wxEVT_THREAD );
    evnt.SetInt( (int)EventType::OPEN_TRANSFER_UI );
    evnt.SetString( remoteId );
    wxQueueEvent( this, evnt.Clone() );
}

int serviceMain( std::promise<void>& promise )
{
    setThreadName( "Main Service Thread" );

    // Initialize protobuf/grpc
    grpc::EnableDefaultHealthCheckService( true );

    auto service = std::make_unique<srv::WinpinatorService>();

    Globals::get()->setWinpinatorServiceInstance( service.get() );

    // Set our promise to true
    promise.set_value();

    service->setGrpcPort( 52000 );
    service->setAuthPort( 52001 );
    int result = service->startOnThisThread();

    Globals::get()->setWinpinatorServiceInstance( nullptr );

    return result;
}
