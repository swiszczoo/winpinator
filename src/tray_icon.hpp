#pragma once

#include <wx/wx.h>
#include <wx/taskbar.h>

wxDECLARE_EVENT( EVT_RESTORE_WINDOW, wxCommandEvent );
wxDECLARE_EVENT( EVT_OPEN_SAVE_FOLDER, wxCommandEvent );
wxDECLARE_EVENT( EVT_EXIT_APP, wxCommandEvent );

class TrayIcon : public wxTaskBarIcon
{
public:
    TrayIcon();

    void setErrorState();
    void setWaitState();
    void setOkState();
    bool isInErrorState() const;
    bool isInWaitState() const;
    bool isInOkState() const;

    void setEventHandler( wxEvtHandler* handler );

protected:
    virtual wxMenu* CreatePopupMenu();

private:
    enum
    {
        ID_RESTORE = 5000,
        ID_OPEN_SAVE_FOLDER,
        ID_EXIT_APP
    };

    enum class State
    {
        ERR,
        WAIT,
        OK
    };

    wxIcon m_icon;
    wxEvtHandler* m_handler;
    State m_state;

    void onClick( wxTaskBarIconEvent& event );
    void onRestoreClicked( wxCommandEvent& event );
    void onOpenSaveFolderClicked( wxCommandEvent& event );
    void onExitAppClicked( wxCommandEvent& event );
};
