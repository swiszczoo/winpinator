#include "status_text.hpp"

#include "utils.hpp"

namespace gui
{

StatusText::StatusText( wxWindow* parent )
    : wxPanel( parent )
    , m_status( srv::RemoteStatus::OFFLINE )
    , m_label( nullptr )
    , m_accentColor( 0, 0, 0 )
    , m_bgColor( 200, 200, 200 )
{
    m_linePen.SetWidth( FromDIP( 6 ) );

    wxBoxSizer* sizer = new wxBoxSizer( wxHORIZONTAL );
    sizer->AddStretchSpacer();

    sizer->AddSpacer( FromDIP( 10 ) );

    m_label = new wxStaticText( this, wxID_ANY, wxEmptyString );
    wxFont fnt = m_label->GetFont();
    fnt.MakeBold();
    m_label->SetFont( fnt );
    sizer->Add( m_label, 0, wxTOP | wxBOTTOM, FromDIP( 3 ) );

    sizer->AddSpacer( FromDIP( 10 ) );

    SetSizer( sizer );

    Bind( wxEVT_PAINT, &StatusText::onPaint, this );
    Bind( wxEVT_SIZE, &StatusText::onSize, this );
    Bind( wxEVT_DPI_CHANGED, &StatusText::onDpiChanged, this );
}

void StatusText::setStatus( srv::RemoteStatus status )
{
    m_status = status;

    if ( status == srv::RemoteStatus::ONLINE )
    {
        setColors( Utils::GREEN_ACCENT, Utils::GREEN_BACKGROUND );
    }
    else if ( status == srv::RemoteStatus::OFFLINE
        || status == srv::RemoteStatus::UNREACHABLE )
    {
        setColors( Utils::RED_ACCENT, Utils::RED_BACKGROUND );
    }
    else
    {
        setColors( Utils::ORANGE_ACCENT, Utils::ORANGE_BACKGROUND );
    }

    m_label->SetLabel( Utils::getStatusString( status ) );

    Layout();
    Refresh();
}

srv::RemoteStatus StatusText::getStatus() const
{
    return m_status;
}

wxColour StatusText::getBarColor() const
{
    return m_accentColor;
}

void StatusText::setColors( wxColour accentColor, wxColour bgColor )
{
    m_accentColor = accentColor;
    m_bgColor = bgColor;

    m_label->SetForegroundColour( accentColor );
    m_label->SetBackgroundColour( bgColor );

    m_rectBrush.SetColour( bgColor );
    m_linePen.SetColour( accentColor );

    Refresh();
}

void StatusText::onPaint( wxPaintEvent& event )
{
    wxPaintDC dc( this );

    int labelWidth = m_label->GetSize().x;
    labelWidth += FromDIP( 10 ) * 2;

    int labelHeight = m_label->GetSize().y;
    labelHeight += FromDIP( 5 );

    dc.SetBrush( m_rectBrush );
    dc.SetPen( *wxTRANSPARENT_PEN );
    dc.DrawRectangle( wxPoint( dc.GetSize().x - labelWidth, 0 ),
        wxSize( labelWidth, labelHeight ) );

    dc.SetPen( m_linePen );
    dc.DrawLine( wxPoint( 0, 0 ), wxPoint( dc.GetSize().x, 0 ) );
}

void StatusText::onSize( wxSizeEvent& event )
{
    Refresh();
    event.Skip( true );
}

void StatusText::onDpiChanged( wxDPIChangedEvent& event )
{
    m_linePen.SetWidth( FromDIP( 6 ) );
    Refresh();
}

};