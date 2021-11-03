#include "history_element_finished.hpp"

#include "../globals.hpp"
#include "utils.hpp"

#include <wx/filename.h>

#include <algorithm>
#include <set>

namespace gui
{

wxDEFINE_EVENT( EVT_REMOVE, wxCommandEvent );

HistoryFinishedElement::HistoryFinishedElement( wxWindow* parent,
    HistoryStdBitmaps* bitmaps )
    : HistoryIconItem( parent, bitmaps )
    , m_bitmaps( bitmaps )
    , m_brush( wxNullBrush )
{
    SetMinSize( FromDIP( wxSize( 16, 76 ) ) );

    m_bold = GetFont();
    m_bold.MakeBold();

    m_brush = wxBrush( *wxBLACK );

    // Events

    Bind( wxEVT_PAINT, &HistoryFinishedElement::onPaint, this );
    Bind( wxEVT_LEFT_DCLICK, &HistoryFinishedElement::onDoubleClicked, this );
    Bind( wxEVT_CONTEXT_MENU, &HistoryFinishedElement::onContextMenu, this );

    Bind( wxEVT_MENU, &HistoryFinishedElement::onOpenClicked,
        this, (int)MenuID::ID_OPEN );
    Bind( wxEVT_MENU, &HistoryFinishedElement::onShowInExplorerClicked,
        this, (int)MenuID::ID_EXPLORER );
    Bind( wxEVT_MENU, &HistoryFinishedElement::onShowListClicked,
        this, (int)MenuID::ID_PATHS );
    Bind( wxEVT_MENU, &HistoryFinishedElement::onRemoveClicked,
        this, (int)MenuID::ID_REMOVE );
}

void HistoryFinishedElement::setData( const TransferData& newData )
{
    m_data = newData;

    setIcons( newData.folderCount,
        newData.fileCount, newData.singleElementName );
    setOutcoming( newData.outgoing );
    setFinished( newData.outgoing || newData.transferType != srv::db::TransferType::UNFINISHED_INCOMING );
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

    // Draw operation status

    dc.SetFont( m_bold );
    dc.SetPen( *wxTRANSPARENT_PEN );

    wxString statusLabel = determineStatusString();
    const int LABEL_MARGIN = FromDIP( 8 );
    const int LABEL_HEIGHT = FromDIP( 20 );
    wxSize labelExtent = dc.GetTextExtent( statusLabel );
    int labelWidth = 2 * LABEL_MARGIN + m_bitmaps->statusSuccess.GetWidth()
        + labelExtent.x + FromDIP( 4 );
    int labelX = size.x - LABEL_MARGIN - labelWidth;
    const int LABEL_X = labelX;

    const wxBitmap& labelBitmap = setupStatusDrawing( &dc );
    dc.DrawRectangle( labelX, ( size.y - LABEL_HEIGHT ) / 2,
        labelWidth, LABEL_HEIGHT );

    labelX += LABEL_MARGIN;

    dc.DrawBitmap( labelBitmap,
        wxPoint( labelX, ( size.y - labelBitmap.GetHeight() ) / 2 ) );

    labelX += FromDIP( 4 ) + labelBitmap.GetWidth();

    dc.DrawText( statusLabel, labelX, ( size.y - labelExtent.y ) / 2 );

    // Draw operation heading

    dc.SetFont( Utils::get()->getHeaderFont() );
    dc.SetTextForeground( GetForegroundColour() );

    int topContentWidth = size.x - contentOffsetX - FromDIP( 10 );
    int contentWidth = LABEL_X - contentOffsetX - FromDIP( 10 );
    int offsetY = FromDIP( 6 );

    Utils::drawTextEllipse( dc, determineHeaderString(),
        wxPoint( contentOffsetX, offsetY ), topContentWidth );

    offsetY += dc.GetTextExtent( "A" ).y + 4;

    // Draw details labels
    // Column 1
    int colWidth = size.x / 2 - contentOffsetX - FromDIP( 6 );

    dc.SetFont( GetFont() );
    dc.SetTextForeground( GRAY );

    const wxString typeLabel = _( "Type:" );
    const wxString sizeLabel = _( "Total size:" );

    int typeWidth = dc.GetTextExtent( typeLabel ).x;
    int sizeWidth = dc.GetTextExtent( sizeLabel ).x;
    int columnWidth = std::max( sizeWidth, typeWidth );
    int lineHeight = dc.GetTextExtent( "A" ).y + 4;

    int detailsWidth = colWidth - columnWidth - FromDIP( 4 );

    if ( detailsWidth > 0 )
    {
        dc.DrawText( typeLabel,
            contentOffsetX, offsetY );
        dc.DrawText( sizeLabel,
            contentOffsetX, offsetY + lineHeight );

        dc.SetTextForeground( GetForegroundColour() );

        int typeX = contentOffsetX + FromDIP( 5 ) + typeWidth;
        int sizeX = contentOffsetX + FromDIP( 5 ) + sizeWidth;

        Utils::drawTextEllipse( dc, getElementType(),
            wxPoint( typeX, offsetY ),
            contentOffsetX + colWidth - FromDIP( 4 ) - typeX );

        Utils::drawTextEllipse( dc, Utils::fileSizeToString( m_data.totalSizeBytes ),
            wxPoint( sizeX, offsetY + lineHeight ),
            contentOffsetX + colWidth - FromDIP( 4 ) - sizeX );
    }

    // Column 2
    int col2Width = LABEL_X - size.x / 2 - FromDIP( 6 );
    dc.SetTextForeground( GRAY );

    int col2Offset = size.x / 2;

    const wxString startDateLabel = _( "Date and time:" );

    int startDateWidth = dc.GetTextExtent( startDateLabel ).x;

    int details2Width = col2Width - startDateWidth - FromDIP( 4 );

    if ( details2Width > 0 )
    {
        dc.DrawText( startDateLabel,
            col2Offset, offsetY );

        dc.SetTextForeground( GetForegroundColour() );

        Utils::drawTextEllipse( dc,
            // TRANSLATORS: date format string
            Utils::formatDate( m_data.transferTimestamp,
                _( "%Y-%m-%d %I:%M %p" ).ToStdString() ),
            wxPoint( col2Offset, offsetY + lineHeight ),
            LABEL_X - FromDIP( 4 ) - col2Offset );
    }
}

void HistoryFinishedElement::onDoubleClicked( wxMouseEvent& event )
{
    openElement();
}

void HistoryFinishedElement::onContextMenu( wxContextMenuEvent& event )
{
    wxMenu menu;

    menu.Append( (int)MenuID::ID_OPEN, _( "&Open " ) );
    menu.Append( (int)MenuID::ID_EXPLORER, _( "Show in &Explorer..." ) );
    menu.Append( (int)MenuID::ID_PATHS, _( "Show &list of transferred files..." ) );
    menu.AppendSeparator();
    menu.Append( (int)MenuID::ID_REMOVE, _( "&Remove from history" ) );

    bool openAvailable = ( m_data.fileCount == 1 && m_data.folderCount == 0 )
        || ( m_data.fileCount == 0 && m_data.folderCount == 1 );

    menu.Enable( (int)MenuID::ID_OPEN, openAvailable );

    PopupMenu( &menu );
}

void HistoryFinishedElement::onOpenClicked( wxCommandEvent& event )
{
    openElement();
}

void HistoryFinishedElement::onShowInExplorerClicked( wxCommandEvent& event )
{
    auto serv = Globals::get()->getWinpinatorServiceInstance();
    auto transfer = serv->getDb()->getTransfer( m_data.id, m_data.targetId,
        true );

    std::vector<wxFileName> existingElements;
    std::vector<wxArrayString> existingDirs;

    wxString volume;
    wxArrayString longestCommon;
    std::set<wxString> elementsInCommon;

    int minDirs = INT_MAX;

    for ( const auto& element : transfer.elements )
    {
        if ( wxDirExists( element.absolutePath ) )
        {
            wxFileName fname = wxFileName::DirName( element.absolutePath );

            if ( volume == wxEmptyString )
            {
                volume = fname.GetVolume();
            }

            if ( fname.GetVolume() == volume )
            {
                existingElements.push_back( fname );
                existingDirs.push_back( fname.GetDirs() );
                minDirs = std::min( minDirs, (int)fname.GetDirCount() - 1 );
            }
        }
        else if ( wxFileExists( element.absolutePath ) )
        {
            wxFileName fname( element.absolutePath );

            if ( volume == wxEmptyString )
            {
                volume = fname.GetVolume();
            }

            if ( fname.GetVolume() == volume )
            {
                existingElements.push_back( fname );
                existingDirs.push_back( fname.GetDirs() );
                minDirs = std::min( minDirs, (int)fname.GetDirCount() );
            }
        }
    }

    if ( minDirs == INT_MAX )
    {
        return;
    }

    for ( int i = 0; i < minDirs; i++ )
    {
        bool ok = true;
        wxString iterName = wxEmptyString;

        for ( int j = 0; j < existingDirs.size(); j++ )
        {
            if ( iterName.empty() )
            {
                iterName = existingDirs[j][i];
            }

            if ( existingDirs[j][i] != iterName )
            {
                ok = false;
                break;
            }
        }

        if ( ok )
        {
            longestCommon.Add( iterName );
        }
        else
        {
            break;
        }
    }

    for ( auto& element : existingElements )
    {
        if ( element.GetDirCount() > longestCommon.GetCount() )
        {
            elementsInCommon.insert( element.GetDirs()[longestCommon.GetCount()] );
        }
        else
        {
            elementsInCommon.insert( element.GetFullName() );
        }
    }

    wxString longestCommonJoined = wxEmptyString;
    for ( const auto& segment : longestCommon )
    {
        longestCommonJoined += '/';
        longestCommonJoined += segment;
    }

    wxFileName common( volume, longestCommonJoined, "", "" );
    std::vector<wxString> commonElements;
    for ( auto& element : elementsInCommon )
    {
        commonElements.push_back( element );
    }

    Utils::openExplorerWithSelectedFiles( common.GetFullPath(), commonElements );
}

void HistoryFinishedElement::onShowListClicked( wxCommandEvent& event )
{
}

void HistoryFinishedElement::onRemoveClicked( wxCommandEvent& event )
{
    auto serv = Globals::get()->getWinpinatorServiceInstance();
    serv->getDb()->deleteTransfer( m_data.id );

    wxCommandEvent evnt( EVT_REMOVE );
    wxPostEvent( this, evnt );
}

wxString HistoryFinishedElement::determineStatusString()
{
    switch ( m_data.status )
    {
    case srv::db::TransferStatus::SUCCEEDED:
        return _( "Completed" );
    case srv::db::TransferStatus::CANCELLED:
        return _( "Cancelled" );
    case srv::db::TransferStatus::FAILED:
        return _( "Error occurred" );
    default:
        return _( "Unknown" );
    }
}

const wxBitmap& HistoryFinishedElement::setupStatusDrawing( wxDC* dc )
{
    wxBitmap* toReturn;

    switch ( m_data.status )
    {
    case srv::db::TransferStatus::SUCCEEDED:
        m_brush.SetColour( Utils::GREEN_BACKGROUND );
        dc->SetTextForeground( Utils::GREEN_ACCENT );
        toReturn = &m_bitmaps->statusSuccess;
        break;
    case srv::db::TransferStatus::CANCELLED:
        m_brush.SetColour( Utils::ORANGE_BACKGROUND );
        dc->SetTextForeground( Utils::ORANGE_ACCENT );
        toReturn = &m_bitmaps->statusCancelled;
        break;
    case srv::db::TransferStatus::FAILED:
        m_brush.SetColour( Utils::RED_BACKGROUND );
        dc->SetTextForeground( Utils::RED_ACCENT );
        toReturn = &m_bitmaps->statusError;
        break;
    default:
        m_brush.SetColour( wxColour( 160, 160, 160 ) );
        dc->SetTextForeground( *wxBLACK );
        toReturn = &m_bitmaps->statusCancelled;
    }

    dc->SetBrush( m_brush );

    return *toReturn;
}

void HistoryFinishedElement::openElement()
{
    auto serv = Globals::get()->getWinpinatorServiceInstance();
    auto transfer = serv->getDb()->getTransfer( m_data.id, m_data.targetId,
        true );

    bool succeed = false;

    if ( transfer.fileCount == 0 && transfer.folderCount == 1 )
    {

        for ( const auto& element : transfer.elements )
        {
            if ( element.elementType == srv::db::TransferElementType::FOLDER )
            {
                wxFileName dirName = wxFileName::DirName( element.absolutePath );

                if ( dirName.DirExists() )
                {
                    Utils::openDirectoryInExplorer( dirName.GetFullPath() );
                    succeed = true;
                }

                break;
            }
        }
    }
    else if ( transfer.fileCount == 1 && transfer.folderCount == 0 )
    {
        if ( transfer.elements.size() == 1 )
        {
            auto element = transfer.elements[0];

            wxFileName elementName( element.absolutePath );

            if ( elementName.FileExists() )
            {
                wxString fullPath = elementName.GetFullPath();
                ShellExecuteW( NULL, L"open",
                    fullPath.wc_str(), NULL, NULL, SW_SHOWNORMAL );
                succeed = true;
            }
        }
    }

    if ( !succeed )
    {
        wxMessageBox(
            _( "Selected element does no longer exist on the hard drive." ),
            _( "Can't open element" ), wxICON_EXCLAMATION );
    }
}

};
