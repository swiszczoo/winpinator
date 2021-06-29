#include "winpinator_app.hpp"

#include "winpinator_frame.hpp"


namespace gui
{

WinpinatorApp::WinpinatorApp()
    : m_locale( wxLANGUAGE_ENGLISH_US )
{
}

bool WinpinatorApp::OnInit()
{
    wxInitAllImageHandlers();

    SetAppName( wxT( "Winpinator" ) );

    WinpinatorFrame* frame = new WinpinatorFrame( nullptr );
    frame->Show( true );

    return true;
}

};
