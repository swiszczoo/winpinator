#include "winpinator_app.hpp"

#include "winpinator_frame.hpp"


namespace gui
{

WinpinatorApp::WinpinatorApp()
{
}

bool WinpinatorApp::OnInit()
{
    wxInitAllImageHandlers();

    WinpinatorFrame* frame = new WinpinatorFrame( nullptr );
    frame->Show( true );

    return true;
}

};
