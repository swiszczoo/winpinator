#include "globals.hpp"
#include "gui/winpinator_frame.hpp"
#include "service/winpinator_service.hpp"

#include <google/protobuf/message_lite.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <wx/wx.h>

#include <stdlib.h>
#include <Windows.h>

#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <thread>

int serviceMain( std::promise<void>& promise );

class WinpinatorApp : public wxApp
{
public:
    WinpinatorApp();
    virtual bool OnInit() override;
    virtual int OnExit() override;

private:
    wxLocale m_locale;
    wxTopLevelWindow* m_topLvl;
    std::thread m_srvThread;

    void showMainFrame();
    void onMainFrameDestroyed( wxWindowDestroyEvent& event );
};

wxIMPLEMENT_APP( WinpinatorApp );

WinpinatorApp::WinpinatorApp()
    : m_locale( wxLANGUAGE_ENGLISH_US )
    , m_topLvl( nullptr )
{
}

bool WinpinatorApp::OnInit()
{
    wxInitAllImageHandlers();

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

    // Show main window
    showMainFrame();

    return true;
}

int WinpinatorApp::OnExit()
{
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
    m_topLvl = nullptr;
}

int serviceMain( std::promise<void>& promise )
{
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
