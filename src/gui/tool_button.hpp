#pragma once
#include <wx/wx.h>


namespace gui
{

class ToolButton : public wxButton
{
public:
    ToolButton( wxWindow* parent, wxWindowID id,
        const wxString& label = wxEmptyString,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = 0,
        const wxValidator& validator = wxDefaultValidator,
        const wxString& name = wxASCII_STR( wxButtonNameStr ) );

private:
    bool m_hovering;
    bool m_pressed;
    bool m_focus;

    void onPaint( wxPaintEvent& event );
    bool MSWOnDraw( WXDRAWITEMSTRUCT* wxdis );
};

};
