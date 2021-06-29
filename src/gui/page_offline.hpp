#pragma once
#include <wx/wx.h>


namespace gui
{

class OfflinePage : public wxPanel
{
public:
    OfflinePage( wxWindow* parent );

private:
    static wxString s_text;

    wxStaticBitmap* m_icon;
    wxStaticText* m_label;
    wxBitmap m_iconBmp;

    void onDpiChanged( wxDPIChangedEvent& event );

    void loadIcon();
};

};
