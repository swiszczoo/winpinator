#pragma once
#include "../service/service_observer.hpp"
#include "about_dialog.hpp"
#include "file_list_dialog.hpp"
#include "screen_selector.hpp"
#include "settings_dialog.hpp"
#include "winpinator_banner.hpp"

#include <wx/wx.h>

#include <memory>

namespace gui
{

wxDECLARE_EVENT( EVT_EXIT_APP_FROM_FRAME, wxCommandEvent );

class WinpinatorFrame : public wxFrame, srv::IServiceObserver
{
public:
    explicit WinpinatorFrame( wxWindow* parent );

    bool showTransferScreen( const wxString& remoteId );
    void killAllDialogs();
    void putOnTop();

    void setTransferList( const std::vector<wxString>& paths );

private:
    static const wxString RELEASE_NOTES_FILENAME;

    wxMenuBar* m_menuBar;
    wxMenu* m_fileMenu;
    wxMenu* m_helpMenu;
    wxStatusBar* m_statusBar;

    WinpinatorBanner* m_banner;
    ScreenSelector* m_selector;

    AboutDialog* m_aboutDlg;
    SettingsDialog* m_settingsDlg;
    std::shared_ptr<FileListDialog> m_fileListDlg;

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
    void onUpdateBannerQueueSize( wxCommandEvent& event );

    void onDialogOpened( PointerEvent& event );
    void onDialogClosed( PointerEvent& event );

    virtual void onStateChanged();
    virtual void onIpAddressChanged( std::string newIp );
};

};
