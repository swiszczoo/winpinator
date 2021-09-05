#pragma once
#include <wx/wx.h>

namespace gui
{

/*
  This is the base class for all entries that show up in the 
  transfer history box. It provides implementation for common
  behaviors, like drawing list item background on mouse hover.
  */
class HistoryItem : public wxPanel
{
public:
    HistoryItem( wxWindow* parent );
    void updateHoverState( bool insideParent );

protected:
    void eraseNullSink( wxEraseEvent& event );

private:
    bool m_hovering;

    void refreshHoverState();
    void onErase( wxEraseEvent& event );
};

};
