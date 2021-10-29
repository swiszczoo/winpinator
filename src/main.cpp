#include "main.hpp"

#include "globals.hpp"
#include "service/service_observer.hpp"
#include "service/winpinator_service.hpp"
#include "thread_name.hpp"
#include "tray_icon.hpp"
#include "winpinator_dde_client.hpp"

#include <google/protobuf/message_lite.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <wx/log.h>
#include <wx/msw/regconf.h>
#include <wx/socket.h>
#include <wx/stdpaths.h>
#include <wx/wx.h>

#include <Windows.h>
#include <stdlib.h>

#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <thread>

int serviceMain( std::promise<void>& promise, SettingsModel settings );

wxIMPLEMENT_APP( WinpinatorApp );

WinpinatorApp::WinpinatorApp()
    : m_topLvl( nullptr )
    , m_trayIcon( nullptr )
    , m_detector( nullptr )
    , m_ddeServer( nullptr )
{
    m_config = std::unique_ptr<wxConfigBase>( wxConfigBase::Create() );
    m_settings.loadFrom( m_config.get() );

    m_locale.AddCatalog( "locales" );

    bool langInitialized = false;
    const wxLanguageInfo* langInfo = wxLocale::FindLanguageInfo( 
        m_settings.localeName );

    if ( langInfo )
    {
        if ( wxLocale::IsAvailable( langInfo->Language ) )
        {
            m_locale.Init( langInfo->Language );
            langInitialized = true;
        }
    }

    if ( !langInitialized )
    {
        // Fall back to English (United States)
        m_locale.Init( wxLANGUAGE_ENGLISH_US );
    }

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

    // Make AppData subdirectory
    wxMkDir( wxStandardPaths::Get().GetUserDataDir() );

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
        std::ref( promise ), m_settings ) );

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
    if ( m_settings.openWindowOnStart )
    {
        showMainFrame();
    }

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

    // Save settings
    m_settings.saveTo( m_config.get() );

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
    else
    {
        m_topLvl->putOnTop();
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
        m_topLvl->killAllDialogs();
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

int serviceMain( std::promise<void>& promise, SettingsModel settings )
{
    setThreadName( "Main Service Thread" );

    // Initialize protobuf/grpc
    grpc::EnableDefaultHealthCheckService( true );

    auto service = std::make_unique<srv::WinpinatorService>();

    Globals::get()->setWinpinatorServiceInstance( service.get() );

    // Set our promise to true
    promise.set_value();

    int result = service->startOnThisThread( settings );

    Globals::get()->setWinpinatorServiceInstance( nullptr );

    return result;
}
