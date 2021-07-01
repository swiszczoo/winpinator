#include "host_listbox.hpp"

#include "../../win32/resource.h"
#include "utils.hpp"

#include <wx/msw/uxtheme.h>

namespace gui
{

HostListbox::HostListbox( wxWindow* parent )
    : wxVListBox( parent, wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxLB_MULTIPLE )
    , m_items( {} )
    , m_defaultPic( wxNullBitmap )
    , m_rowHeight( 0 )
    , m_hotItem( -1 )
{
    SetItemCount( 3 );
    SetWindowTheme( GetHWND(), L"Explorer", NULL );

    calcRowHeight();
    loadIcons();

    // Events

    Bind( wxEVT_DPI_CHANGED, &HostListbox::onDpiChanged, this );
    Bind( wxEVT_ENTER_WINDOW, &HostListbox::onMouseMotion, this );
    Bind( wxEVT_LEAVE_WINDOW, &HostListbox::onMouseMotion, this );
    Bind( wxEVT_MOTION, &HostListbox::onMouseMotion, this );
    Bind( wxEVT_MOUSEWHEEL, &HostListbox::onMouseMotion, this );
}

void HostListbox::OnDrawItem( wxDC& dc, const wxRect& rect, size_t n ) const
{
    const int MARGIN = FromDIP( 10 );
    const int TMARGIN = FromDIP( 8 );
    const int SPACING = FromDIP( 4 );
    const int IMG_SIZE = FromDIP( 64 );
    wxColour GRAY = wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT );
    wxColour BLACK = wxSystemSettings::GetColour(
        wxSYS_COLOUR_INACTIVECAPTIONTEXT );

    if ( !wxUxThemeIsActive() && IsSelected( n ) )
    {
        GRAY = wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHTTEXT );
        BLACK = wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHTTEXT );
    }

    const int offX = rect.x;
    const int offY = rect.y;

    HostItem item;
    item.hostname = "lukasz@DESK-X570";
    item.ipAddress = "192.168.1.27";
    item.os = "Linux";
    item.username = wxT( "£ukasz Œwiszcz" );
    item.state = CONNECTED;

    // Force non-const this pointer
    HostListbox* ncthis = const_cast<HostListbox*>( this );

    // Draw profile picture
    if ( item.profilePic.IsOk() )
    {
        if ( !item.profileBmp || !item.profileBmp->IsOk() )
        {
            scaleProfilePic( item );
        }

        int topMargin = ( m_rowHeight - item.profileBmp->GetSize().y ) / 2;

        dc.DrawBitmap( *item.profileBmp,
            wxPoint( MARGIN + offX, topMargin + offY ) );
    }
    else
    {
        int topMargin = ( m_rowHeight - ncthis->m_defaultPic.GetSize().y ) / 2;

        dc.DrawBitmap( m_defaultPic,
            wxPoint( MARGIN + offX, topMargin + offY ) );
    }

    // Draw username
    dc.SetTextForeground( BLACK );
    dc.SetFont( Utils::get()->getHeaderFont() );
    dc.DrawText( item.username,
        wxPoint( 2 * MARGIN + IMG_SIZE + offX, TMARGIN + offY ) );

    // Calculate text offsets
    const int OFF1 = TMARGIN + offY
        + dc.GetTextExtent( wxT( "AAA" ) ).y + SPACING;

    dc.SetFont( GetFont() );

    const int OFF2 = OFF1 + SPACING + dc.GetTextExtent( wxT( "AAA" ) ).y;

    const int maxWidth = ( rect.width - 2 * MARGIN - IMG_SIZE ) / 2 - SPACING;

    // Draw first column ( hostname + IP )
    int column1 = offX + 2 * MARGIN + IMG_SIZE;
    dc.SetTextForeground( GRAY );

    const wxString hostnameLabel = _( "Hostname:" );
    const wxString ipLabel = _( "IP Address:" );

    int hostnameWidth = dc.GetTextExtent( hostnameLabel ).x;
    int ipWidth = dc.GetTextExtent( ipLabel ).x;
    int maxC1 = std::max( hostnameWidth, ipWidth );

    dc.DrawText( hostnameLabel,
        wxPoint( column1 + maxC1 - hostnameWidth, OFF1 ) );
    dc.DrawText( ipLabel,
        wxPoint( column1 + maxC1 - ipWidth, OFF2 ) );

    dc.SetTextForeground( BLACK );

    Utils::drawTextEllipse( dc, item.hostname,
        wxPoint( column1 + maxC1 + SPACING, OFF1 ),
        maxWidth - SPACING - maxC1 );
    Utils::drawTextEllipse( dc, item.ipAddress,
        wxPoint( column1 + maxC1 + SPACING, OFF2 ),
        maxWidth - SPACING - maxC1 );

    // Draw second column ( OS + state )
    int column2 = column1 + maxWidth + SPACING;
    dc.SetTextForeground( GRAY );

    const wxString osLabel = _( "OS:" );
    const wxString stateLabel = _( "Connection:" );

    int osWidth = dc.GetTextExtent( osLabel ).x;
    int stateWidth = dc.GetTextExtent( stateLabel ).x;
    int maxC2 = std::max( osWidth, stateWidth );

    dc.DrawText( osLabel,
        wxPoint( column2 + maxC2 - osWidth, OFF1 ) );
    dc.DrawText( stateLabel,
        wxPoint( column2 + maxC2 - stateWidth, OFF2 ) );

    wxString stateText;
    switch ( item.state )
    {
    case HostState::LOADING:
        stateText = _( "Processing..." );
        break;
    case HostState::HOST_UNREACHABLE:
        stateText = _( "Host is unreachable" );
        break;
    case HostState::CONNECTING:
        stateText = _( "Connecting..." );
        break;
    case HostState::CONNECTED:
        stateText = _( "Ready" );
        break;
    default:
        stateText = _( "Unknown" );
        break;
    }

    dc.SetTextForeground( BLACK );

    Utils::drawTextEllipse( dc, item.os,
        wxPoint( column2 + maxC2 + SPACING, OFF1 ),
        maxWidth - SPACING - maxC2 );
    Utils::drawTextEllipse( dc, stateText,
        wxPoint( column2 + maxC2 + SPACING, OFF2 ),
        maxWidth - SPACING - maxC2 );
}

void HostListbox::OnDrawBackground( wxDC& dc,
    const wxRect& rect, size_t n ) const
{
    // TODO: stub!
    if ( wxUxThemeIsActive() )
    {
        wxUxThemeHandle theme( this, L"LISTVIEW" );

        int partId = LVP_LISTITEM;
        int stateId = LISS_NORMAL;

        if ( m_hotItem == n )
        {
            stateId = LISS_HOT;
        }
        if ( IsSelected( n ) )
        {
            stateId = LISS_SELECTED;
        }
        if ( IsSelected( n ) && m_hotItem == n )
        {
            stateId = LISS_HOTSELECTED;
        }

        WXHDC winDc = dc.GetHDC();
        RECT dcRect;
        dcRect.left = rect.GetX();
        dcRect.top = rect.GetY();
        dcRect.right = rect.GetRight();
        dcRect.bottom = rect.GetBottom();

        if ( stateId != LISS_NORMAL && stateId != LISS_DISABLED )
        {
            DrawThemeBackground( theme, winDc, partId, stateId, &dcRect, NULL );
        }

        if ( HasFocus() && IsSelected( n ) )
        {
            //DrawFocusRect( winDc, &dcRect );
        }
    }
    else
    {
        // If UXTHEME is not active, fallback to default bg implementation
        wxVListBox::OnDrawBackground( dc, rect, n );
    }
}

wxCoord HostListbox::OnMeasureItem( size_t n ) const
{
    return m_rowHeight;
}

void HostListbox::onDpiChanged( wxDPIChangedEvent& event )
{
    calcRowHeight();
    loadIcons();
    RefreshAll();
}

void HostListbox::onMouseMotion( wxMouseEvent& event )
{
    if ( event.GetWheelDelta() )
    {
        HandleOnMouseWheel( event );
    }

    if ( event.Moving() || event.GetWheelDelta() )
    {
        wxPoint pnt = event.GetPosition();
        size_t firstItem = GetVisibleRowsBegin();
        int offset = pnt.y / m_rowHeight;
        int nextHotItem = firstItem + offset;

        if ( m_hotItem != nextHotItem )
        {
            if ( m_hotItem >= 0 && m_hotItem < GetItemCount() )
            {
                RefreshRow( m_hotItem );
            }

            RefreshRow( nextHotItem );

            m_hotItem = nextHotItem;
        }
    }
    else if ( event.Leaving() )
    {
        int lastItem = m_hotItem;
        m_hotItem = -1;

        if ( lastItem >= 0 && lastItem < GetItemCount() )
        {
            RefreshRow( lastItem );
        }
    }
}

void HostListbox::loadIcons()
{
    wxBitmap original;
    original.LoadFile( Utils::makeIntResource( IDB_UNK_PROFILE ),
        wxBITMAP_TYPE_PNG_RESOURCE );

    wxImage toScale = original.ConvertToImage();

    int size = FromDIP( 64 );
    m_defaultPic = toScale.Scale( size, size, wxIMAGE_QUALITY_BICUBIC );
}

void HostListbox::calcRowHeight()
{
    wxCoord totalHeight = 0;

    wxClientDC dc( this );
    dc.SetFont( Utils::get()->getHeaderFont() );
    totalHeight += dc.GetTextExtent( wxT( "AAA" ) ).y; // Header line

    dc.SetFont( GetFont() );
    totalHeight += 2 * dc.GetTextExtent( wxT( "AAA" ) ).y; // Two details lines

    totalHeight += 2 * FromDIP( 4 ); // Margins between lines of text
    totalHeight += 2 * FromDIP( 10 ); // Top and bottom padding

    m_rowHeight = totalHeight;
}

bool HostListbox::scaleProfilePic( HostItem& item ) const
{
    int targetDim = FromDIP( 64 );

    if ( !item.profilePic.IsOk() )
    {
        return false;
    }

    item.profileBmp = std::make_shared<wxBitmap>(
        item.profilePic.Scale( targetDim, targetDim, wxIMAGE_QUALITY_BICUBIC ) );

    return true;
}

};
