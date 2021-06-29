#pragma once
#include <wx/wx.h>
#include <wx/vlbox.h>


namespace gui
{

class HostListbox : public wxVListBox
{
public:
    HostListbox( wxWindow* parent );

protected:
    virtual void OnDrawItem( wxDC& dc, const wxRect& rect, size_t n ) const;
    virtual void OnDrawBackground( wxDC& dc, 
        const wxRect& rect, size_t n ) const;
    virtual wxCoord OnMeasureItem( size_t n ) const;
};

};
