#include "gui/winpinator_app.hpp"

#include "globals.hpp"
#include "service/winpinator_service.hpp"

#include <stdlib.h>
#include <Windows.h>

#include <google/protobuf/message_lite.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <thread>


int serviceMain( int argc, char* argv[], std::promise<void>& promise );
int uiMain( int argc, char* argv[], std::future<void>& future );


int genericMain( int argc, char* argv[] )
{
    // Run two threads - the first one running background services
    // (zeroconf, grpc, etc.) and the second one running graphical
    // user interface

    // We must ensure that ui thread waits until global service pointer
    // becomes valid. This can be accomplished with std::promise and
    // std::future

    std::promise<void> promise;
    std::future<void> future = promise.get_future();

    std::thread srvThread( std::bind( serviceMain, 
        argc, argv, std::ref(promise) ) );
    std::thread uiThread( std::bind( uiMain, argc, argv, std::ref(future) ) );
    
    // Wait for both of them to finish before exiting the process

    srvThread.join();
    uiThread.join();

    // Shut down protobuf
    google::protobuf::ShutdownProtobufLibrary();

    return EXIT_SUCCESS;
}

int serviceMain( int argc, char* argv[], std::promise<void>& promise )
{
    // Initialize protobuf/grpc
    grpc::EnableDefaultHealthCheckService( true );
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();

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

int uiMain( int argc, char* argv[], std::future<void>& future )
{
    // Wait for service pointer to become valid
    future.wait();

    gui::WinpinatorApp* app = new gui::WinpinatorApp();
    gui::WinpinatorApp::SetInstance( app );

    wxEntry( argc, argv );
    wxEntryCleanup();

    return EXIT_SUCCESS;
}

#ifdef _WIN32

INT WINAPI WinMain( _In_ HINSTANCE hInstance, 
    _In_opt_ HINSTANCE hPrevInstance, 
    _In_ LPSTR lpCmdLine, 
    _In_ int nShowCmd )
{
    return genericMain( __argc, __argv );
}

#else

int main(int argc, char* argv[])
{
    return genericMain( argc, argv );
}

#endif
