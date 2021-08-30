#pragma once
#include <wx/wx.h>

namespace gui
{

class HistoryGroupHeader : public wxPanel
{
public:
    explicit HistoryGroupHeader( wxWindow* parent, const wxString& label );

private:
    wxStaticText* m_label;
    wxPen m_pen;

    void onPaint( wxPaintEvent& event );
    void onSize( wxSizeEvent& event );
};

};
