#include "history_element_finished.hpp"

#include "utils.hpp"

namespace gui
{

HistoryFinishedElement::HistoryFinishedElement( wxWindow* parent, 
    HistoryStdBitmaps* bitmaps )
    : HistoryIconItem( parent, bitmaps )
    , m_bitmaps( bitmaps )
{
    SetMinSize( FromDIP( wxSize( 16, 76 ) ) );

    // Events

    Bind( wxEVT_PAINT, &HistoryFinishedElement::onPaint, this );
    Bind( wxEVT_CONTEXT_MENU, &HistoryFinishedElement::onContextMenu, this );
}

void HistoryFinishedElement::setData( const TransferData& newData )
{
    m_data = newData;

    setIcons( newData.folderCount, 
        newData.fileCount, newData.singleElementName );
    setOutcoming( newData.outgoing );
}

const TransferData& HistoryFinishedElement::getData() const
{
    return m_data;
}

void HistoryFinishedElement::onPaint( wxPaintEvent& event )
{
    wxPaintDC dc( this );
    wxSize size = dc.GetSize();
    wxColour GRAY = wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT );

    wxCoord contentOffsetX = drawIcon( dc );

    // Draw operation heading

    dc.SetFont( Utils::get()->getHeaderFont() );
    dc.SetTextForeground( GetForegroundColour() );

    int contentWidth = dc.GetSize().x - contentOffsetX - FromDIP( 10 );
    int offsetY = FromDIP( 6 );

    Utils::drawTextEllipse( dc, determineHeaderString(),
        wxPoint( contentOffsetX, offsetY ), contentWidth );

    offsetY += dc.GetTextExtent( "A" ).y + FromDIP( 4 );

    // Draw details labels

    dc.SetFont( GetFont() );
    dc.SetTextForeground( GRAY );

    const wxString sizeLabel = _( "Total size:" );
    const wxString startDateLabel = _( "Transfer date:" );

    int sizeWidth = dc.GetTextExtent( sizeLabel ).x;
    int startDateWidth = dc.GetTextExtent( startDateLabel ).x;
    int columnWidth = std::max( sizeWidth, startDateWidth );
    int lineHeight = dc.GetTextExtent( "A" ).y + FromDIP( 4 );

    int detailsWidth = contentWidth - columnWidth - FromDIP( 4 );

    if ( detailsWidth > 0 )
    {
        dc.DrawText( startDateLabel,
            contentOffsetX + columnWidth - startDateWidth, offsetY );
        dc.DrawText( sizeLabel,
            contentOffsetX + columnWidth - sizeWidth, offsetY + lineHeight );
    }

    dc.SetTextForeground( GetForegroundColour() );

    int detailsX = columnWidth + contentOffsetX + FromDIP( 4 );

    Utils::drawTextEllipse( dc,
        // TRANSLATORS: date format string
        Utils::formatDate( m_data.transferTimestamp,
            _( "%Y-%m-%d %I:%M %p" ).ToStdString() ),
        wxPoint( detailsX, offsetY ), detailsWidth );
    Utils::drawTextEllipse( dc, Utils::fileSizeToString( m_data.totalSizeBytes ),
        wxPoint( detailsX, offsetY + lineHeight ), detailsWidth );
}

void HistoryFinishedElement::onContextMenu(wxContextMenuEvent& event)
{
    wxMenu menu;

    menu.Append( (int)MenuID::ID_EXPLORER, _( "Show in Explorer" ) );
    menu.Append( (int)MenuID::ID_PATHS, _( "Show list of transferred files" ) );
    menu.AppendSeparator();
    menu.Append( (int)MenuID::ID_REMOVE, _( "Remove from history" ) );

    PopupMenu( &menu );
}

};
