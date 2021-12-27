#include "winpinator_frame.hpp"

#include "../../win32/resource.h"
#include "../globals.hpp"
#include "../main_base.hpp"
#include "about_dialog.hpp"
#include "history_element_finished.hpp"
#include "utils.hpp"

#include <wx/stdpaths.h>

#define ID_OPENDESTDIR 10000
#define ID_RELEASENOTES 10001

namespace gui
{

wxDEFINE_EVENT( EVT_EXIT_APP_FROM_FRAME, wxCommandEvent );

const wxString WinpinatorFrame::RELEASE_NOTES_FILENAME = "RELEASE NOTES.txt";

WinpinatorFrame::WinpinatorFrame( wxWindow* parent )
    : wxFrame( parent, wxID_ANY, _( "Winpinator" ) )
    , m_menuBar( nullptr )
    , m_fileMenu( nullptr )
    , m_helpMenu( nullptr )
    , m_statusBar( nullptr )
    , m_banner( nullptr )
    , m_aboutDlg( nullptr )
    , m_settingsDlg( nullptr )
    , m_fileListDlg( nullptr )
{
    SetSize( FromDIP( 800 ), FromDIP( 620 ) );
    SetMinSize( FromDIP( wxSize( 610, 400 ) ) );

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

    m_selector = new ScreenSelector( this );
    mainSizer->Add( m_selector, 1, wxEXPAND, 0 );

    SetSizer( mainSizer );

    m_statusBar = new wxStatusBar( this, wxID_ANY );

    int statusWidths[] = { -1, FromDIP( 130 ) };
    m_statusBar->SetFieldsCount( sizeof( statusWidths ) / sizeof( int ) );
    m_statusBar->SetStatusWidths( m_statusBar->GetFieldsCount(), statusWidths );

    srv::WinpinatorService* serv = Globals::get()->getWinpinatorServiceInstance();

    m_statusBar->SetStatusText( serv->getDisplayName(), 0 );

    if ( serv->isOnline() )
    {
        // TRANSLATORS: format string
        m_statusBar->SetStatusText(
            wxString::Format( _( "IP: %s" ), serv->getIpAddress() ), 1 );
    }
    else
    {
        m_statusBar->SetStatusText( _( "Offline!" ), 1 );
    }

    SetStatusBar( m_statusBar );

    observeService( Globals::get()->getWinpinatorServiceInstance() );

    // onAboutSelected();

    // Events
    Bind( wxEVT_MENU, &WinpinatorFrame::onMenuItemSelected, this );
    Bind( wxEVT_THREAD, &WinpinatorFrame::onChangeStatusBarText, this );
    Bind( wxEVT_DPI_CHANGED, &WinpinatorFrame::onDpiChanged, this );
    m_selector->Bind( EVT_UPDATE_BANNER_TARGET,
        &WinpinatorFrame::onUpdateBannerTarget, this );
    m_selector->Bind( EVT_OPEN_SETTINGS,
        &WinpinatorFrame::onSettingsClicked, this );
    m_selector->Bind( EVT_UPDATE_BANNER_QSIZE,
        &WinpinatorFrame::onUpdateBannerQueueSize, this );
    Bind( EVT_OPEN_DIALOG, &WinpinatorFrame::onDialogOpened, this );
    Bind( EVT_CLOSE_DIALOG, &WinpinatorFrame::onDialogClosed, this );
}

bool WinpinatorFrame::showTransferScreen( const wxString& remoteId )
{
    putOnTop();
    return m_selector->showTransferScreen( remoteId );
}

void WinpinatorFrame::killAllDialogs()
{
    if ( m_aboutDlg )
    {
        m_aboutDlg->EndModal( wxID_CANCEL );
    }

    if ( m_settingsDlg )
    {
        m_settingsDlg->EndModal( wxID_CANCEL );
    }

    if ( m_fileListDlg )
    {
        m_fileListDlg->EndModal( wxID_CANCEL );
    }
}

void WinpinatorFrame::putOnTop()
{
    Iconize( false );
    SetFocus();
    Raise();
    Show( true );
}

void WinpinatorFrame::setTransferList( const std::vector<wxString>& list )
{
    m_selector->setTransferList( list );
}

void WinpinatorFrame::setupMenuBar()
{
    m_menuBar = new wxMenuBar();

    m_fileMenu = new wxMenu();
    m_fileMenu->Append( ID_OPENDESTDIR, _( "&Open save folder...\tCtrl+O" ),
        _( "Open the incoming files folder in Explorer" ) );
    m_fileMenu->Append( wxID_PREFERENCES, _( "&Preferences..." ),
        _( "Adjust app preferences" ) );
    m_fileMenu->AppendSeparator();
    m_fileMenu->Append( wxID_CLOSE_FRAME, _( "E&xit\tAlt+F4" ),
        _( "Close this window but let Winpinator run in background" ) );
    m_fileMenu->Append( wxID_EXIT,
        _( "Exit and &stop the service\tCtrl+Alt+F4" ),
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

void WinpinatorFrame::onChangeStatusBarText( wxThreadEvent& event )
{
    m_statusBar->SetStatusText( event.GetString(), 1 );
}

void WinpinatorFrame::onDpiChanged( wxDPIChangedEvent& event )
{
    int statusWidths[] = { -1, FromDIP( 130 ) };
    m_statusBar->SetFieldsCount( sizeof( statusWidths ) / sizeof( int ) );
    m_statusBar->SetStatusWidths( m_statusBar->GetFieldsCount(), statusWidths );
}

void WinpinatorFrame::onSettingsClicked( wxCommandEvent& event )
{
    onPrefsSelected();
}

void WinpinatorFrame::onOpenFolderSelected()
{
    const SettingsModel& settings = GetApp().m_settings;
    Utils::openDirectoryInExplorer( settings.outputPath );
}

void WinpinatorFrame::onPrefsSelected()
{
    m_settingsDlg = new SettingsDialog( this );
    if ( m_settingsDlg->ShowModal() == wxID_OK )
    {
        // Restart service
        srv::Event evnt;
        evnt.type = srv::EventType::RESTART_SERVICE;
        evnt.eventData.restartData = std::make_shared<SettingsModel>(
            GetApp().m_settings );

        auto serv = Globals::get()->getWinpinatorServiceInstance();
        serv->postEvent( evnt );
    }
    m_settingsDlg->Destroy();
    m_settingsDlg = nullptr;
}

void WinpinatorFrame::onCloseSelected()
{
    Close();
}

void WinpinatorFrame::onExitSelected()
{
    wxCommandEvent evnt( EVT_EXIT_APP_FROM_FRAME );
    wxPostEvent( this, evnt );
}

void WinpinatorFrame::onHelpSelected()
{
    wxMessageBox( _( "Sorry, but Winpinator help is not available yet." ),
        _( "Help not available" ), wxICON_INFORMATION );
}

void WinpinatorFrame::onShowReleaseNotesSelected()
{
    wxFileName notesName( wxStandardPaths::Get().GetExecutablePath() );
    notesName.SetFullName( RELEASE_NOTES_FILENAME );

    wxLaunchDefaultApplication( notesName.GetFullPath() );
}

void WinpinatorFrame::onAboutSelected()
{
    m_aboutDlg = new AboutDialog( this );
    m_aboutDlg->ShowModal();
    m_aboutDlg->Destroy();
    m_aboutDlg = nullptr;
}

void WinpinatorFrame::onUpdateBannerTarget( PointerEvent& event )
{
    m_banner->setTargetInfo( event.getSharedPointer<srv::RemoteInfo>() );
}

void WinpinatorFrame::onUpdateBannerQueueSize( wxCommandEvent& event )
{
    m_banner->setSendQueueSize( event.GetInt() );
}

void WinpinatorFrame::onDialogOpened( PointerEvent& event )
{
    m_fileListDlg = event.getSharedPointer<FileListDialog>();
}

void WinpinatorFrame::onDialogClosed( PointerEvent& event )
{
    if ( m_fileListDlg == event.getSharedPointer<FileListDialog>() )
    {
        m_fileListDlg = nullptr;
    }
}

void WinpinatorFrame::onStateChanged()
{
    srv::WinpinatorService* serv = Globals::get()->getWinpinatorServiceInstance();

    if ( !serv->isOnline() )
    {
        wxThreadEvent evnt( wxEVT_THREAD );
        evnt.SetString( _( "Offline!" ) );
        wxQueueEvent( this, evnt.Clone() );
    }
}

void WinpinatorFrame::onIpAddressChanged( std::string newIp )
{
    wxThreadEvent evnt( wxEVT_THREAD );

    if ( newIp.empty() )
    {
        evnt.SetString( _( "Offline!" ) );
    }
    else
    {
        // TRANSLATORS: format string
        evnt.SetString( wxString::Format( _( "IP: %s" ), newIp ) );
    }

    wxQueueEvent( this, evnt.Clone() );
}

};
