#include "winpinator_frame.hpp"

#include "../../win32/resource.h"
#include "page_hostlist.hpp"
#include "utils.hpp"

#include <wx/aboutdlg.h>


#define ID_OPENDESTDIR 10000
#define ID_RELEASENOTES 10001

namespace gui
{

WinpinatorFrame::WinpinatorFrame( wxWindow* parent )
    : wxFrame( parent, wxID_ANY, _( "Winpinator" ) )
    , m_menuBar( nullptr )
    , m_fileMenu( nullptr )
    , m_helpMenu( nullptr )
    , m_statusBar( nullptr )
    , m_banner( nullptr )
{
    SetSize( FromDIP( 800 ), FromDIP( 600 ) );
    SetMinSize( FromDIP( wxSize( 500, 400 ) ) );

    wxIconBundle icons( Utils::makeIntResource( IDI_WINPINATOR ), 
        GetModuleHandle( NULL ) );
    SetIcons( icons );

    CenterOnScreen();
    SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

    setupMenuBar();
    setupAccelTable();

    wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );

    m_banner = new WinpinatorBanner( this, 80 );
    mainSizer->Add( m_banner, 0, wxEXPAND, 0 );

    HostListPage* p = new HostListPage( this );
    mainSizer->Add( p, 1, wxEXPAND, 0 );

    SetSizer( mainSizer );


    m_statusBar = new wxStatusBar( this, wxID_ANY );

    int statusWidths[] = { -1, 130 };
    m_statusBar->SetFieldsCount( sizeof( statusWidths ) / sizeof( int ) );
    m_statusBar->SetStatusWidths( m_statusBar->GetFieldsCount(), statusWidths );
    m_statusBar->SetStatusText( wxT( "username@hostname" ), 0 );
    m_statusBar->SetStatusText( wxT( "IP: 192.168.1.1" ), 1 );
    
    SetStatusBar( m_statusBar );

    // Events
    Bind( wxEVT_MENU, &WinpinatorFrame::onMenuItemSelected, this );
}

void WinpinatorFrame::setupMenuBar()
{
    m_menuBar = new wxMenuBar();

    m_fileMenu = new wxMenu();
    m_fileMenu->Append( ID_OPENDESTDIR, _( "Open save folder...\tCtrl+O" ), 
        _( "Open the incoming files folder in Explorer" ) );
    m_fileMenu->Append( wxID_PREFERENCES, _( "Preferences..." ),
        _( "Adjust app preferences" ) );
    m_fileMenu->AppendSeparator();
    m_fileMenu->Append( wxID_CLOSE_FRAME, _( "Exit\tAlt+F4" ),
        _( "Close this window but let Winpinator run in background" ) );
    m_fileMenu->Append( wxID_EXIT, 
        _( "Exit and stop the service\tCtrl+Alt+F4" ),
        _( "Exit Winpinator and stop being visible to other computers" ) );

    m_menuBar->Append( m_fileMenu, _( "&File" ) );

    m_helpMenu = new wxMenu();
    m_helpMenu->Append( wxID_HELP, _( "&Help topics...\tF1" ),
        _( "Show help and documentation of Winpinator" ) );
    m_helpMenu->Append( ID_RELEASENOTES, _( "&What's new?..." ),
        _( "Show release notes" ) );
    m_helpMenu->Append( wxID_ABOUT, _( "&About Winpinator..." ),
        _( "Show credits dialog" ) );

    m_menuBar->Append( m_helpMenu, _( "&Help" ) );

    SetMenuBar( m_menuBar );
}

void WinpinatorFrame::setupAccelTable()
{
    wxAcceleratorEntry entries[2];
    entries[0].Set( wxACCEL_CTRL, 'O', ID_OPENDESTDIR );
    entries[1].Set( 0, WXK_F1, wxID_HELP );

    wxAcceleratorTable table( 2, entries );
    SetAcceleratorTable( table );
}

void WinpinatorFrame::onMenuItemSelected( wxCommandEvent& event )
{
    switch ( event.GetId() )
    {
    case ID_OPENDESTDIR:
        onOpenFolderSelected();
        break;

    case wxID_PREFERENCES:
        onPrefsSelected();
        break;

    case wxID_CLOSE_FRAME:
        onCloseSelected();
        break;

    case wxID_EXIT:
        onExitSelected();
        break;

    case wxID_HELP:
        onHelpSelected();
        break;

    case ID_RELEASENOTES:
        onShowReleaseNotesSelected();
        break;

    case wxID_ABOUT:
        onAboutSelected();
        break;
    }
}

void WinpinatorFrame::onOpenFolderSelected()
{
}

void WinpinatorFrame::onPrefsSelected()
{
}

void WinpinatorFrame::onCloseSelected()
{
    Close();
}

void WinpinatorFrame::onExitSelected()
{
    Close();
}

void WinpinatorFrame::onHelpSelected()
{
}

void WinpinatorFrame::onShowReleaseNotesSelected()
{
}

void WinpinatorFrame::onAboutSelected()
{
    wxAboutDialogInfo info;

    info.SetName( _( "Winpinator" ) );
    info.SetVersion( "0.1.0" );
    info.SetDescription( _( "Winpinator is an unofficial port of an excellent "
                            "file transfer tool Warpinator for Windows" ) );
    info.SetCopyright( _( "\u00a92021 £ukasz Œwiszcz" ) );

    wxAboutBox( info, this );
}

};
