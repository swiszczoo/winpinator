#pragma once
#include "../service/service_observer.hpp"
#include "screen_selector.hpp"
#include "settings_dialog.hpp"
#include "winpinator_banner.hpp"

#include <wx/wx.h>

#include <memory>

namespace gui
{

class WinpinatorFrame : public wxFrame, srv::IServiceObserver
{
public:
    explicit WinpinatorFrame( wxWindow* parent );

    bool showTransferScreen( const wxString& remoteId );
    void killAllDialogs();
    void putOnTop();

private:
    wxMenuBar* m_menuBar;
    wxMenu* m_fileMenu;
    wxMenu* m_helpMenu;
    wxStatusBar* m_statusBar;

    WinpinatorBanner* m_banner;
    ScreenSelector* m_selector;
    SettingsDialog* m_settingsDlg;

    void setupMenuBar();
    void setupAccelTable();

    void onMenuItemSelected( wxCommandEvent& event );
    void onChangeStatusBarText( wxThreadEvent& event );
    void onDpiChanged( wxDPIChangedEvent& event );
    void onSettingsClicked( wxCommandEvent& event );

    void onOpenFolderSelected();
    void onPrefsSelected();
    void onCloseSelected();
    void onExitSelected();
    void onHelpSelected();
    void onShowReleaseNotesSelected();
    void onAboutSelected();

    void onUpdateBannerTarget( PointerEvent& event );

    virtual void onStateChanged();
    virtual void onIpAddressChanged( std::string newIp );
};

};
