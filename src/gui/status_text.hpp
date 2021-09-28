#pragma once
#include <wx/wx.h>

#include "../service/remote_info.hpp"

namespace gui
{

class StatusText : public wxPanel
{
public:
    explicit StatusText( wxWindow* panel );

    void setStatus( srv::RemoteStatus status );
    srv::RemoteStatus getStatus() const;

    wxColour getBarColor() const;

private:
    srv::RemoteStatus m_status;

    wxStaticText* m_label;

    wxColour m_accentColor;
    wxColour m_bgColor;

    wxBrush m_rectBrush;
    wxPen m_linePen;

    void setColors( wxColour accentColor, wxColour bgColor );

    void onPaint( wxPaintEvent& event );
    void onSize( wxSizeEvent& event );
    void onDpiChanged( wxDPIChangedEvent& event );
};

};
