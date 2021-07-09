#pragma once
#include <wx/wx.h>

#include "host_listbox.hpp"
#include "tool_button.hpp"


namespace gui
{

class HostListPage : public wxPanel
{
public:
    explicit HostListPage( wxWindow* parent );

private:
    static const wxString s_details;
    static const wxString s_detailsWrapped;

    wxStaticText* m_header;
    wxStaticText* m_details;
    ToolButton* m_refreshBtn;
    HostListbox* m_hostlist;
    wxButton* m_fwdBtn;

    wxBitmap m_refreshBmp;

    void onDpiChanged( wxDPIChangedEvent& event );
    void onLabelResized( wxSizeEvent& event );

    void loadIcon();
};

};
