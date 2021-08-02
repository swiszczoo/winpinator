#include "tray_icon.hpp"

#include "../win32/resource.h"
#include "gui/utils.hpp"

#include <wx/translation.h>

wxDEFINE_EVENT( EVT_RESTORE_WINDOW, wxCommandEvent );
wxDEFINE_EVENT( EVT_OPEN_SAVE_FOLDER, wxCommandEvent );
wxDEFINE_EVENT( EVT_EXIT_APP, wxCommandEvent );

TrayIcon::TrayIcon()
    : wxTaskBarIcon( wxTBI_DEFAULT_TYPE )
    , m_icon( wxNullIcon )
    , m_handler( this )
    , m_state( State::WAIT )
{
    setWaitState();

    // Events

    Bind( wxEVT_TASKBAR_LEFT_UP, &TrayIcon::onClick, this );
    Bind( wxEVT_MENU, &TrayIcon::onRestoreClicked, this );
    Bind( wxEVT_MENU, &TrayIcon::onOpenSaveFolderClicked, 
        this, ID_OPEN_SAVE_FOLDER );
    Bind( wxEVT_MENU, &TrayIcon::onExitAppClicked, this, ID_EXIT_APP );
}

void TrayIcon::setErrorState()
{
    m_state = State::ERR;
    m_icon = wxIcon( gui::Utils::makeIntResource( IDI_TRAY_ERROR ) );

    SetIcon( m_icon, _( "Winpinator (Error!)" ) );
}

void TrayIcon::setWaitState()
{
    m_state = State::WAIT;
    m_icon.LoadFile( gui::Utils::makeIntResource( IDI_TRAY_WAIT ) );

    SetIcon( m_icon, _( "Winpinator (Waiting...)" ) );
}

void TrayIcon::setOkState()
{
    m_state = State::OK;
    m_icon.LoadFile( gui::Utils::makeIntResource( IDI_TRAY_OK ) );

    SetIcon( m_icon, _( "Winpinator (Running)" ) );
}

bool TrayIcon::isInErrorState() const
{
    return m_state == State::ERR;
}

bool TrayIcon::isInWaitState() const
{
    return m_state == State::WAIT;
}

bool TrayIcon::isInOkState() const
{
    return m_state == State::OK;
}

void TrayIcon::setEventHandler( wxEvtHandler* handler )
{
    m_handler = handler;
}

wxMenu* TrayIcon::CreatePopupMenu()
{
    wxMenu* menu = new wxMenu;

    menu->Append( ID_RESTORE, _( "&Restore window" ), 
        _( "Restore main app window" ) );
    menu->AppendSeparator();
    menu->Append( ID_OPEN_SAVE_FOLDER, _( "&Open save folder" ), 
        _( "Open the incoming files folder in Explorer" ) );
    menu->Append( ID_EXIT_APP, _( "E&xit" ), 
        _( "Exit Winpinator and stop background service" ) );

    return menu;
}

void TrayIcon::onClick( wxTaskBarIconEvent& event )
{
    wxCommandEvent newEvent( EVT_RESTORE_WINDOW );
    wxPostEvent( this, newEvent );
}

void TrayIcon::onRestoreClicked( wxCommandEvent& event )
{
    wxCommandEvent newEvent( EVT_RESTORE_WINDOW );
    wxPostEvent( m_handler, newEvent );
}

void TrayIcon::onOpenSaveFolderClicked( wxCommandEvent& event )
{
    wxCommandEvent newEvent( EVT_OPEN_SAVE_FOLDER );
    wxPostEvent( m_handler, newEvent );
}

void TrayIcon::onExitAppClicked( wxCommandEvent& event )
{
    wxCommandEvent newEvent( EVT_EXIT_APP );
    wxPostEvent( m_handler, newEvent );
}
