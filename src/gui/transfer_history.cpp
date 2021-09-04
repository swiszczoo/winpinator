#include "transfer_history.hpp"

#include "utils.hpp"

#include <wx/translation.h>

namespace gui
{

const std::vector<wxString> ScrolledTransferHistory::TIME_SPECS = {
    _( "Today" ),
    _( "Yesterday" ),
    _( "Earlier this week" ),
    _( "Last week" ),
    _( "Earlier this month" ),
    _( "Last month" ),
    _( "Earlier this year" ),
    _( "Last year" ),
    _( "A long time ago" )
};

ScrolledTransferHistory::ScrolledTransferHistory( wxWindow* parent )
    : wxScrolledWindow( parent, wxID_ANY )
{
    SetWindowStyle( wxBORDER_NONE | wxVSCROLL );
    SetScrollRate( 0, FromDIP( 8 ) );

    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

    m_emptyLabel = new wxStaticText( this, wxID_ANY, _( "There is no transfer "
        "history for this device..." ) );
    m_emptyLabel->SetForegroundColour( 
        wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );

    sizer->Add( m_emptyLabel, 0, 
        wxALIGN_CENTER_HORIZONTAL | wxALL, FromDIP( 12 ) );

    for ( const wxString& spec : ScrolledTransferHistory::TIME_SPECS )
    {
        HistoryGroupHeader* header = new HistoryGroupHeader( this, spec );
        sizer->Add( header, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP( 3 ) );

        wxPanel* panel = new wxPanel( this );
        wxBoxSizer* panelSizer = new wxBoxSizer( wxVERTICAL );
        panel->SetSizer( panelSizer );

        sizer->Add( panel, 0, wxEXPAND );

        m_timeHeaders.push_back( header );
        m_timeGroups.push_back( panel );
        m_timeSizers.push_back( panelSizer );
        registerHistoryItem( header );
    }

    SetSizer( sizer );
    sizer->SetSizeHints( this );
    SetDropTarget( new DropTargetImpl( this ) );

    // Events
    Bind( wxEVT_SCROLLWIN_BOTTOM,
        &ScrolledTransferHistory::onScrollWindow, this );
    Bind( wxEVT_SCROLLWIN_LINEDOWN,
        &ScrolledTransferHistory::onScrollWindow, this );
    Bind( wxEVT_SCROLLWIN_LINEUP,
        &ScrolledTransferHistory::onScrollWindow, this );
    Bind( wxEVT_SCROLLWIN_PAGEDOWN,
        &ScrolledTransferHistory::onScrollWindow, this );
    Bind( wxEVT_SCROLLWIN_PAGEUP,
        &ScrolledTransferHistory::onScrollWindow, this );
    Bind( wxEVT_SCROLLWIN_THUMBRELEASE, 
        &ScrolledTransferHistory::onScrollWindow, this );
    Bind( wxEVT_SCROLLWIN_THUMBTRACK,
        &ScrolledTransferHistory::onScrollWindow, this );
    Bind( wxEVT_SCROLLWIN_TOP,
        &ScrolledTransferHistory::onScrollWindow, this );
    Bind( wxEVT_ENTER_WINDOW, &ScrolledTransferHistory::onMouseEnter, this );
    Bind( wxEVT_LEAVE_WINDOW, &ScrolledTransferHistory::onMouseLeave, this );
    Bind( wxEVT_MOTION, &ScrolledTransferHistory::onMouseMotion, this );
}

void ScrolledTransferHistory::registerHistoryItem( HistoryItem* item )
{
    m_historyItems.push_back( item );

    item->Bind( wxEVT_ENTER_WINDOW, 
        &ScrolledTransferHistory::onMouseEnter, this );
    item->Bind( wxEVT_LEAVE_WINDOW, 
        &ScrolledTransferHistory::onMouseLeave, this );
    item->Bind( wxEVT_MOTION, &ScrolledTransferHistory::onMouseMotion, this );
}

void ScrolledTransferHistory::refreshAllHistoryItems( bool insideParent )
{
    for ( HistoryItem* item : m_historyItems )
    {
        item->updateHoverState( insideParent );
    }
}

void ScrolledTransferHistory::onScrollWindow( wxScrollWinEvent& event )
{
    refreshAllHistoryItems( true );
    event.Skip( true );
}

void ScrolledTransferHistory::onMouseEnter( wxMouseEvent& event )
{
    refreshAllHistoryItems( true );
    event.Skip( true );
}

void ScrolledTransferHistory::onMouseLeave( wxMouseEvent& event )
{
    bool outside = event.GetEventObject() == this;

    refreshAllHistoryItems( !outside );
    event.Skip( true );
}

void ScrolledTransferHistory::onMouseMotion( wxMouseEvent& event )
{
    refreshAllHistoryItems( true );
    event.Skip( true );
}

// Drop target implementation

ScrolledTransferHistory::DropTargetImpl::DropTargetImpl( 
    ScrolledTransferHistory* obj )
    : m_instance( obj )
{
}

bool ScrolledTransferHistory::DropTargetImpl::OnDropFiles( wxCoord x, 
    wxCoord y, const wxArrayString& filenames )
{
    return true;
}

};
