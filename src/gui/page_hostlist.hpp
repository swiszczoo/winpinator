#pragma once
#include <wx/wx.h>

#include "../service/service_observer.hpp"

#include "host_listbox.hpp"
#include "progress_label.hpp"
#include "tool_button.hpp"

#include <vector>

namespace gui
{

wxDECLARE_EVENT( EVT_NO_HOSTS_IN_TIME, wxCommandEvent );

class HostListPage : public wxPanel, srv::IServiceObserver
{
public:
    explicit HostListPage( wxWindow* parent );

    void refreshAll();

private:
    enum class ThreadEventType
    {
        ADD,
        UPDATE,
        REMOVE,
        RESET
    };

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

    std::vector<srv::RemoteInfoPtr> m_trackedRemotes;

    void onDpiChanged( wxDPIChangedEvent& event );
    void onLabelResized( wxSizeEvent& event );
    void onRefreshClicked( wxCommandEvent& event );

    void onTimerTicked( wxTimerEvent& event );

    void onManipulateList( wxThreadEvent& event );

    void loadIcon();

    static HostItem convertRemoteInfoToHostItem( srv::RemoteInfoPtr rinfo );

    // Observer methods
    virtual void onStateChanged() override;
    virtual void onAddHost( srv::RemoteInfoPtr info ) override;
};

};
