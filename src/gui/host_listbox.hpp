#pragma once
#include "../service/remote_info.hpp"

#include <wx/vlbox.h>
#include <wx/wx.h>

#include <memory>
#include <vector>


namespace gui
{

struct HostItem
{
    std::string id;

    std::shared_ptr<wxImage> profilePic;
    std::shared_ptr<wxBitmap> profileBmp;
    wxString username;
    wxString hostname;
    wxString ipAddress;
    wxString os;
    srv::RemoteStatus state;
};

class HostListbox : public wxVListBox
{
public:
    explicit HostListbox( wxWindow* parent );

    void addItem( const HostItem& item );
    void updateItem( size_t position, const HostItem& item );
    bool updateItemById( const HostItem& newData );
    void removeItem( size_t position );
    void clear();

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
    void onRightClick( wxMouseEvent& event );

    void loadIcons();
    void calcRowHeight();
    bool scaleProfilePic( HostItem& item ) const;
    wxString getStatusString( srv::RemoteStatus status ) const;
};

};
