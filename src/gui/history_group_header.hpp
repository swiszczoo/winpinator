#pragma once
#include "history_item.hpp"

#include <wx/wx.h>

namespace gui
{

class HistoryGroupHeader : public HistoryItem
{
public:
    explicit HistoryGroupHeader( wxWindow* parent, const wxString& label );

    void SetLabel( const wxString& label ) override;
    wxString GetLabel() const override;

private:
    wxString m_label;
    wxPen m_pen;

    void updateSize();

    void onPaint( wxPaintEvent& event );
    void onSize( wxSizeEvent& event );
    void onDpiChanged( wxDPIChangedEvent& event );
};

};
