#pragma once
#include <wx/wx.h>

#include "screen_selector.hpp"
#include "winpinator_banner.hpp"


namespace gui
{

class WinpinatorFrame : public wxFrame
{
public:
    explicit WinpinatorFrame( wxWindow* parent );

private:
    wxMenuBar* m_menuBar;
    wxMenu* m_fileMenu;
    wxMenu* m_helpMenu;
    wxStatusBar* m_statusBar;

    WinpinatorBanner* m_banner;
    ScreenSelector* m_selector;

    void setupMenuBar();
    void setupAccelTable();

    void onMenuItemSelected( wxCommandEvent& event );

    void onOpenFolderSelected();
    void onPrefsSelected();
    void onCloseSelected();
    void onExitSelected();
    void onHelpSelected();
    void onShowReleaseNotesSelected();
    void onAboutSelected();
};

};

