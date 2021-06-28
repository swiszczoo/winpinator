#include "gui/winpinator_app.hpp"

#include <stdlib.h>
#include <Windows.h>

#include <google/protobuf/message_lite.h>

#include <functional>
#include <iostream>
#include <thread>


int serviceMain( int argc, char* argv[] );
int uiMain( int argc, char* argv[] );


int genericMain( int argc, char* argv[] )
{
    // Run two threads - the first one running background services
    // (zeroconf, grpc, etc.) and the second one running graphical
    // user interface

    std::thread srvThread( std::bind( serviceMain, argc, argv ) );
    std::thread uiThread( std::bind( uiMain, argc, argv ) );
    
    // Wait for both of them to finish before exiting the process

    srvThread.join();
    uiThread.join();

    // Shut down protobuf
    google::protobuf::ShutdownProtobufLibrary();

    return EXIT_SUCCESS;
}

int serviceMain( int argc, char* argv[] )
{
    return EXIT_SUCCESS;
}

int uiMain( int argc, char* argv[] )
{
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
