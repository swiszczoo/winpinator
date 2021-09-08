#include "history_group_header.hpp"

#include "utils.hpp"

namespace gui
{

HistoryGroupHeader::HistoryGroupHeader( wxWindow* parent,
    const wxString& label )
    : HistoryItem( parent )
    , m_label( label )
    , m_pen( wxNullPen )
{
    m_pen.SetColour( Utils::get()->getHeaderColor() );
    m_pen.SetWidth( FromDIP( 1 ) );

    updateSize();

    // Events

    Bind( wxEVT_PAINT, &HistoryGroupHeader::onPaint, this );
    Bind( wxEVT_SIZE, &HistoryGroupHeader::onSize, this );
    Bind( wxEVT_DPI_CHANGED, &HistoryGroupHeader::onDpiChanged, this );
}

void HistoryGroupHeader::updateSize()
{
    wxSize height = FromDIP( Utils::get()->getHeaderFont().GetPixelSize() );

    SetMinSize( wxSize( 16, height.GetHeight() * 1.2 + 2 * FromDIP( 3 ) ) );
}

void HistoryGroupHeader::onPaint( wxPaintEvent& event )
{
    wxPaintDC dc( this );
    wxSize size = dc.GetSize();
    const int MARGIN = FromDIP( 8 );

    dc.SetFont( Utils::get()->getHeaderFont() );
    dc.SetTextForeground( Utils::get()->getHeaderColor() );

    wxSize labelExtent = dc.GetTextExtent( m_label );
    wxSize baselineExtent = dc.GetTextExtent( "AAA" );

    int textX = MARGIN;
    int textY = ( size.y - baselineExtent.y ) / 2;

    dc.DrawText( m_label, textX, textY );

    int lineY = size.y / 2;
    int lineX1 = labelExtent.GetWidth() + 2 * MARGIN;
    int lineX2 = size.x - MARGIN;

    dc.SetPen( m_pen );
    dc.DrawLine( lineX1, lineY, lineX2, lineY );
}

void HistoryGroupHeader::onSize( wxSizeEvent& event )
{
    Refresh();
}

void HistoryGroupHeader::onDpiChanged( wxDPIChangedEvent& event )
{
    updateSize();
    Refresh();
}

};
