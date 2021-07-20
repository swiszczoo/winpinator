#pragma once
#include <wx/wx.h>

#include "host_listbox.hpp"
#include "progress_label.hpp"
#include "tool_button.hpp"


namespace gui
{

wxDECLARE_EVENT( EVT_NO_HOSTS_IN_TIME, wxCommandEvent );

class HostListPage : public wxPanel
{
public:
    explicit HostListPage( wxWindow* parent );

    void refreshAll();

private:
    static const wxString s_details;
    static const wxString s_detailsWrapped;
    static const int NO_HOSTS_TIMEOUT_MILLIS;

    wxStaticText* m_header;
    wxStaticText* m_details;
    ToolButton* m_refreshBtn;
    HostListbox* m_hostlist;
    wxButton* m_fwdBtn;
    ProgressLabel* m_progLbl;


    wxBitmap m_refreshBmp;

    wxTimer* m_timer;

    void onDpiChanged( wxDPIChangedEvent& event );
    void onLabelResized( wxSizeEvent& event );
    void onRefreshClicked( wxCommandEvent& event );

    void onTimerTicked( wxTimerEvent& event );

    void loadIcon();
};

};
