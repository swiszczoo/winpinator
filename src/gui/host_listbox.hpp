#pragma once
#include <wx/vlbox.h>
#include <wx/wx.h>

#include <memory>
#include <vector>


namespace gui
{

enum HostState
{
    LOADING,
    HOST_UNREACHABLE,
    CONNECTING,
    CONNECTED
};

struct HostItem
{
    wxImage profilePic;
    std::shared_ptr<wxBitmap> profileBmp;
    wxString username;
    wxString hostname;
    wxString ipAddress;
    wxString os;
    HostState state;
};

class HostListbox : public wxVListBox
{
public:
    HostListbox( wxWindow* parent );

protected:
    virtual void OnDrawItem( wxDC& dc, const wxRect& rect, size_t n ) const;
    virtual void OnDrawBackground( wxDC& dc, 
        const wxRect& rect, size_t n ) const;
    virtual wxCoord OnMeasureItem( size_t n ) const;

private:
    std::vector<HostItem> m_items;
    wxBitmap m_defaultPic;
    wxCoord m_rowHeight;
    int m_hotItem;

    void onDpiChanged( wxDPIChangedEvent& event );
    void onMouseMotion( wxMouseEvent& event );

    void loadIcons();
    void calcRowHeight();
    bool scaleProfilePic( HostItem& item ) const;
};

};
