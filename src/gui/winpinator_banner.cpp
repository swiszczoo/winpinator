#include "winpinator_banner.hpp"

#include "../../win32/resource.h"
#include "utils.hpp"

#include <wx/mstream.h>


namespace gui
{

wxColour WinpinatorBanner::s_gradient1 = wxColour( 55, 124, 200 );
wxColour WinpinatorBanner::s_gradient2 = wxColour( 138, 71, 231 );

WinpinatorBanner::WinpinatorBanner( wxWindow* parent, int height )
    : wxPanel( parent, wxID_ANY )
    , m_height( height )
    , m_headerFont( nullptr )
    , m_detailsFont( nullptr )
    , m_targetMode( false )
    , m_targetName( "" )
    , m_targetDetails( "" )
    , m_targetAvatarImg( wxNullImage )
    , m_targetAvatarBmp( wxNullBitmap )
{
    SetBackgroundColour( wxColour( 0, 0, 0 ) );

    wxSize size = GetSize();
    size.y = height;
    SetSize( size );
    SetMinSize( wxSize( 0, FromDIP( height ) ) );


    m_headerFont = std::make_unique<wxFont>(
        wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT ) );

    m_headerFont->SetPointSize( 22 );

    m_detailsFont = std::make_unique<wxFont>(
        wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT ) );

    m_detailsFont->SetPointSize( 13 );


    loadResources();

    // Events

    Bind( wxEVT_DPI_CHANGED, &WinpinatorBanner::onDpiChanged, this );
    Bind( wxEVT_PAINT, &WinpinatorBanner::onDraw, this );
    Bind( wxEVT_SIZE, &WinpinatorBanner::onSize, this );
}

void WinpinatorBanner::setTargetInfo( srv::RemoteInfoPtr targetPtr )
{
    if ( !targetPtr )
    {
        m_targetMode = false;
        m_targetName = "";
        m_targetDetails = "";
        m_targetAvatarImg = wxNullImage;
        m_targetAvatarBmp = wxNullBitmap;

        Refresh();
        return;
    }

    std::lock_guard<std::mutex> guard( targetPtr->mutex );

    m_targetMode = true;
    m_targetName = targetPtr->fullName;
    m_targetDetails.Printf( "%s@%s / %s", 
        targetPtr->shortName, targetPtr->hostname, targetPtr->ips.ipv4 );

    if ( targetPtr->avatarBuffer.empty() )
    {
        m_targetAvatarImg = wxNullImage;
        m_targetAvatarBmp = wxNullBitmap;
    }
    else
    {
        wxLogNull logNull;

        wxMemoryInputStream stream( targetPtr->avatarBuffer.data(), 
            targetPtr->avatarBuffer.size() );
        m_targetAvatarImg = wxImage( stream, wxBITMAP_TYPE_ANY );

        rescaleTargetAvatar();
    }
    
    Refresh();
}

void WinpinatorBanner::onDpiChanged( wxDPIChangedEvent& event )
{
    loadResources();
    SetMinSize( wxSize( 0, FromDIP( m_height ) ) );
    rescaleTargetAvatar();
    Refresh( true );
}

void WinpinatorBanner::onDraw( wxPaintEvent& event )
{
    wxPaintDC dc( this );

    if ( m_targetMode )
    {
        drawTargetInfo( dc );
    }
    else 
    {
        drawBasic( dc );
    }
}

void WinpinatorBanner::onSize( wxSizeEvent& event )
{
    Refresh( true );
}

void WinpinatorBanner::loadResources()
{
    wxBitmap original;
    original.LoadFile( Utils::makeIntResource( IDB_BLOT ),
        wxBITMAP_TYPE_PNG_RESOURCE );

    wxImage logoImg = original.ConvertToImage();

    float scale = FromDIP( 80 ) / 160.f;

    m_logo = logoImg.Scale( logoImg.GetWidth() * scale, 
        logoImg.GetHeight() * scale, wxIMAGE_QUALITY_BICUBIC );
}

void WinpinatorBanner::rescaleTargetAvatar()
{
    if ( m_targetAvatarImg.IsOk() )
    {
        int targetSize = FromDIP( 64 );

        m_targetAvatarBmp = m_targetAvatarImg.Scale( 
            targetSize, targetSize, wxIMAGE_QUALITY_BICUBIC );
    }
}

void WinpinatorBanner::drawBasic( wxDC& dc )
{
    wxSize size = dc.GetSize();

    dc.GradientFillLinear( wxRect( wxPoint( 0, 0 ), size ),
        s_gradient1, s_gradient2, wxEAST );

    // Draw text

    dc.SetTextForeground( *wxWHITE );
    dc.SetFont( *m_headerFont );

    const wxString text = _( "Share your files across the LAN" );
    wxSize textSize = dc.GetTextExtent( "MM" );

    int textY = (int)( size.y - textSize.y ) / 2;

    dc.DrawText( text, wxPoint( FromDIP( 24 ), textY ) );

    dc.DrawBitmap( m_logo, wxPoint( size.x - m_logo.GetWidth(), 0 ) );
}

void WinpinatorBanner::drawTargetInfo( wxDC& dc )
{
    wxSize size = dc.GetSize();

    dc.GradientFillLinear( wxRect( wxPoint( 0, 0 ), size ),
        s_gradient1, s_gradient2, wxEAST );

    int posX = FromDIP( 24 );

    // Draw avatar if available
    if ( m_targetAvatarBmp.IsOk() )
    {
        int bitmapY = ( size.y - m_targetAvatarBmp.GetSize().y ) / 2;
        dc.DrawBitmap( m_targetAvatarBmp, wxPoint( posX, bitmapY ) );

        posX += m_targetAvatarBmp.GetSize().x + FromDIP( 16 );
    }

    // Calculate text metrics
    dc.SetFont( *m_headerFont );

    int lineHeight1 = dc.GetTextExtent( "MM" ).y;

    dc.SetFont( *m_detailsFont );

    int lineHeight2 = dc.GetTextExtent( "MM" ).y;
    
    const int LINE_SPACING = FromDIP( 1 );
    int totalHeight = lineHeight1 + lineHeight2 + LINE_SPACING;

    // Draw text
    dc.SetTextForeground( *wxWHITE );
    dc.SetFont( *m_headerFont );

    int textY1 = ( size.y - totalHeight ) / 2;
    dc.DrawText( m_targetName, wxPoint( posX, textY1 ) );

    dc.SetFont( *m_detailsFont );

    int textY2 = textY1 + lineHeight1 + LINE_SPACING;
    dc.DrawText( m_targetDetails, wxPoint( posX, textY2 ) );

    dc.DrawBitmap( m_logo, wxPoint( size.x - m_logo.GetWidth(), 0 ) );
}

};
