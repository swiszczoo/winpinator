#pragma once
#include <wx/wx.h>


namespace gui
{

class NoHostsPage : public wxPanel
{
public:
    explicit NoHostsPage( wxWindow* parent );

private:
    static const wxString TEXT;

    wxStaticBitmap* m_icon;
    wxStaticText* m_label;
    wxButton* m_retryBtn;
    wxBitmap m_iconBmp;

    void onDpiChanged( wxDPIChangedEvent& event );
    void onRetryClicked( wxCommandEvent& event );

    void loadIcon();
};

};
