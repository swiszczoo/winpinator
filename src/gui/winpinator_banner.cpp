#include "winpinator_banner.hpp"

#include "../../win32/resource.h"
#include "utils.hpp"


namespace gui
{

wxColour WinpinatorBanner::s_gradient1 = wxColour( 55, 124, 200 );
wxColour WinpinatorBanner::s_gradient2 = wxColour( 138, 71, 231 );

WinpinatorBanner::WinpinatorBanner( wxWindow* parent, int height )
    : wxPanel( parent, wxID_ANY )
    , m_height( height )
    , m_headerFont( nullptr )
{
    SetBackgroundColour( wxColour( 0, 0, 0 ) );

    wxSize size = GetSize();
    size.y = height;
    SetSize( size );
    SetMinSize( wxSize( 0, FromDIP( height ) ) );


    m_headerFont = std::make_unique<wxFont>(
        wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT ) );

    m_headerFont->SetPointSize( 22 );


    loadResources();

    // Events

    Bind( wxEVT_DPI_CHANGED, &WinpinatorBanner::onDpiChanged, this );
    Bind( wxEVT_PAINT, &WinpinatorBanner::onDraw, this );
    Bind( wxEVT_SIZE, &WinpinatorBanner::onSize, this );
}

void WinpinatorBanner::onDpiChanged( wxDPIChangedEvent& event )
{
    loadResources();
    SetMinSize( wxSize( 0, FromDIP( m_height ) ) );
    Refresh( true );
}

void WinpinatorBanner::onDraw( wxPaintEvent& event )
{
    wxPaintDC dc( this );
    draw( dc );
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

void WinpinatorBanner::draw( wxDC& dc )
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

};
