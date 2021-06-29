#include "host_listbox.hpp"


namespace gui
{

HostListbox::HostListbox( wxWindow* parent )
    : wxVListBox( parent, wxID_ANY, wxDefaultPosition, 
        wxDefaultSize, wxLB_MULTIPLE )
{

}

void HostListbox::OnDrawItem( wxDC& dc, const wxRect& rect, size_t n ) const
{
    // TODO: stub!
}

void HostListbox::OnDrawBackground( wxDC& dc, 
    const wxRect& rect, size_t n ) const
{
    // TODO: stub!
}

wxCoord HostListbox::OnMeasureItem( size_t n ) const
{
    // TODO: stub!
    return 0;
}

};
