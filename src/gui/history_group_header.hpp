#pragma once
#include "history_item.hpp"

#include <wx/wx.h>

namespace gui
{

class HistoryGroupHeader : public HistoryItem
{
public:
    explicit HistoryGroupHeader( wxWindow* parent, const wxString& label );

private:
    wxStaticText* m_label;
    wxPen m_pen;

    void onPaint( wxPaintEvent& event );
    void onSize( wxSizeEvent& event );
    void ignoreErase( wxEraseEvent& event );
};

};
