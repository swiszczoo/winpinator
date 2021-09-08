#include "transfer_history.hpp"

#include "history_element_pending.hpp"

#include "../../win32/resource.h"
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
    , m_emptyLabel( nullptr )
    , m_stdBitmaps()
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

    // "In progress" ops view

    m_pendingHeader = new HistoryGroupHeader( this, _( "In progress" ) );
    sizer->Add( m_pendingHeader, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP( 5 ) );

    m_pendingPanel = new wxPanel( this );
    m_pendingSizer = new wxBoxSizer( wxVERTICAL );
    m_pendingPanel->SetSizer( m_pendingSizer );

    sizer->Add( m_pendingPanel, 0, wxEXPAND | wxBOTTOM, FromDIP( 5 ) );
    registerHistoryItem( m_pendingHeader );

    HistoryPendingElement* test = new HistoryPendingElement( 
        m_pendingPanel, &m_stdBitmaps );
    test->setPeerName( "FirstName Surname" );
    m_pendingSizer->Add( test, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP( 11 ) );
    registerHistoryItem( test );

    // Historical ops view

    for ( const wxString& spec : ScrolledTransferHistory::TIME_SPECS )
    {
        HistoryGroupHeader* header = new HistoryGroupHeader( this, spec );
        sizer->Add( header, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP( 5 ) );

        wxPanel* panel = new wxPanel( this );
        wxBoxSizer* panelSizer = new wxBoxSizer( wxVERTICAL );
        panel->SetSizer( panelSizer );

        sizer->Add( panel, 0, wxEXPAND | wxBOTTOM, FromDIP( 2 ) );

        m_timeHeaders.push_back( header );
        m_timeGroups.push_back( panel );
        m_timeSizers.push_back( panelSizer );
        registerHistoryItem( header );
    }

    SetSizer( sizer );
    sizer->SetSizeHints( this );
    SetDropTarget( new DropTargetImpl( this ) );

    reloadStdBitmaps();

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

void ScrolledTransferHistory::reloadStdBitmaps()
{
    loadSingleBitmap( IDB_TRANSFER_FILE_X, 
        &m_stdBitmaps.transferFileX, 64 );
    loadSingleBitmap( IDB_TRANSFER_FILE_FILE, 
        &m_stdBitmaps.transferFileFile, 64 );
    loadSingleBitmap( IDB_TRANSFER_DIR_X,
        &m_stdBitmaps.transferDirX, 64 );
    loadSingleBitmap( IDB_TRANSFER_DIR_DIR,
        &m_stdBitmaps.transferDirDir, 64 );
    loadSingleBitmap( IDB_TRANSFER_DIR_FILE,
        &m_stdBitmaps.transferDirFile, 64 );

    loadSingleBitmap( IDB_TRANSFER_UP,
        &m_stdBitmaps.badgeUp, 20 );
    loadSingleBitmap( IDB_TRANSFER_DOWN,
        &m_stdBitmaps.badgeDown, 20 );
}

void ScrolledTransferHistory::loadSingleBitmap( 
    int resId, wxBitmap* dest, int dip )
{
    wxBitmap orig( Utils::makeIntResource( resId ), 
        wxBITMAP_TYPE_PNG_RESOURCE );

    wxImage img = orig.ConvertToImage();

    int targetSize = FromDIP( dip );

    *dest = img.Scale( targetSize, targetSize, wxIMAGE_QUALITY_BICUBIC );
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

void ScrolledTransferHistory::onDpiChanged( wxDPIChangedEvent& event )
{
    reloadStdBitmaps();

    for ( HistoryItem* item : m_historyItems )
    {
        item->Refresh();
    }
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
