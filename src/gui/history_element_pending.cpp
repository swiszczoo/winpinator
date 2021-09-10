#include "history_element_pending.hpp"
#include "utils.hpp"

#include <wx/filename.h>
#include <wx/mimetype.h>

#include <Windows.h>

namespace gui
{

const int HistoryPendingElement::ICON_SIZE = 64;

HistoryPendingElement::HistoryPendingElement( wxWindow* parent,
    HistoryStdBitmaps* bitmaps )
    : HistoryItem( parent )
    , m_info( nullptr )
    , m_infoLabel( "" )
    , m_buttonSizer( nullptr )
    , m_infoCancel( nullptr )
    , m_infoAllow( nullptr )
    , m_infoReject( nullptr )
    , m_infoPause( nullptr )
    , m_infoStop( nullptr )
    , m_infoOverwrite( nullptr )
    , m_bitmaps( bitmaps )
    , m_data()
    , m_peerName( wxEmptyString )
    , m_fileIcon( wxNullIcon )
    , m_fileIconLoc()
{
    wxBoxSizer* horzSizer = new wxBoxSizer( wxHORIZONTAL );

    horzSizer->AddStretchSpacer( 3 );

    // Await peer approval
    m_info = new wxBoxSizer( wxVERTICAL );
    horzSizer->Add( m_info, 2, wxEXPAND | wxLEFT | wxRIGHT, FromDIP( 8 ) );

    m_info->AddStretchSpacer( 1 );

    m_infoProgress = new wxGauge( this, wxID_ANY, 1000000 );
    m_infoProgress->SetMinSize( FromDIP( wxSize( 16, 16 ) ) );
    m_infoProgress->Hide();
    m_info->Add( m_infoProgress, 0, wxEXPAND | wxBOTTOM, FromDIP( 3 ) );

    wxFont guiFont = GetFont();
    m_infoSpacing = FromDIP( guiFont.GetPixelSize().GetHeight() ) * 1.2
        + FromDIP( 6 );
    m_info->AddSpacer( m_infoSpacing );

    m_buttonSizer = new wxBoxSizer( wxHORIZONTAL );

    m_infoCancel = new wxButton( this, wxID_ANY, _( "&Cancel" ) );
    m_buttonSizer->Add( m_infoCancel, 0, wxEXPAND | wxRIGHT, FromDIP( 2 ) );

    m_infoAllow = new wxButton( this, wxID_ANY, _( "&Accept" ) );
    m_buttonSizer->Add( m_infoAllow, 0, wxEXPAND | wxRIGHT, FromDIP( 2 ) );

    m_infoReject = new wxButton( this, wxID_ANY, _( "&Reject" ) );
    m_buttonSizer->Add( m_infoReject, 0, wxEXPAND | wxRIGHT, FromDIP( 2 ) );

    m_infoPause = new wxButton( this, wxID_ANY, wxEmptyString );
    m_buttonSizer->Add( m_infoPause, 0, wxEXPAND | wxRIGHT, FromDIP( 2 ) );

    m_infoStop = new wxButton( this, wxID_ANY, _( "&Stop" ) );
    m_buttonSizer->Add( m_infoStop, 0, wxEXPAND | wxRIGHT, FromDIP( 2 ) );

    m_infoOverwrite = new wxButton( this, wxID_ANY, _( "Over&write" ) );
    m_buttonSizer->Add( m_infoOverwrite, 0, wxEXPAND );

    m_info->Add( m_buttonSizer, 0, wxALIGN_CENTER_HORIZONTAL );

    m_info->AddStretchSpacer( 1 );

    SetSizer( horzSizer );
    SetMinSize( FromDIP( wxSize( 16, 76 ) ) );

    setupForState( HistoryPendingState::AWAIT_MY_APPROVAL );

    // Events

    Bind( wxEVT_PAINT, &HistoryPendingElement::onPaint, this );
    Bind( wxEVT_DPI_CHANGED, &HistoryPendingElement::onDpiChanged, this );
}

void HistoryPendingElement::setData( const HistoryPendingData& newData )
{
    m_data = newData;
    m_fileIcon = wxNullIcon;
    m_fileIconLoc = wxIconLocation();

    if ( newData.numFiles == 1 && newData.numFolders == 0 )
    {
        // If this transfer is a single file, try to load its extension icon

        assert( newData.filePaths.size() == 1 );

        wxFileName fileName( newData.filePaths[0] );
        wxString extension = fileName.GetExt();

        wxIconLocation loc;
        wxFileType* fileType = wxTheMimeTypesManager->GetFileTypeFromExtension( extension );

        if ( fileType )
        {
            wxLogNull logNull;

            if ( fileType->GetIcon( &loc ) && wxFileExists( loc.GetFileName() ) )
            {
                m_fileIcon = Utils::extractIconWithSize(
                    loc, FromDIP( ICON_SIZE ) );
                m_fileIconLoc = loc;
            }

            if ( !m_fileIcon.IsOk() )
            {
                // If something failed, fall back to standard file icon
                m_fileIcon = wxNullIcon;
                m_fileIconLoc = wxIconLocation();
            }

            delete fileType;
        }
    }
}

const HistoryPendingData& HistoryPendingElement::getData() const
{
    return m_data;
}

void HistoryPendingElement::setPeerName( const wxString& peerName )
{
    m_peerName = peerName;

    // Set up GUI
    setupForState( m_data.opState );
}

const wxString& HistoryPendingElement::getPeerName() const
{
    return m_peerName;
}

void HistoryPendingElement::calculateLayout()
{
    wxCoord width = GetTextExtent( m_infoLabel ).x;
    wxCoord margins = FromDIP( 8 ) * 2;

    m_info->SetMinSize( width + margins, FromDIP( 16 ) );

    Refresh();
}

void HistoryPendingElement::setupForState( HistoryPendingState state )
{
    m_infoProgress->Hide();
    m_infoCancel->Hide();
    m_infoAllow->Hide();
    m_infoReject->Hide();
    m_infoPause->Hide();
    m_infoStop->Hide();
    m_infoOverwrite->Hide();

    switch ( state )
    {
    case HistoryPendingState::AWAIT_MY_APPROVAL:
        // TRANSLATORS: %s stands for full name of the peer
        m_infoLabel.Printf( _( "%s is sending you files:" ), m_peerName );

        m_infoAllow->Show();
        m_infoReject->Show();
        break;

    case HistoryPendingState::AWAIT_PEER_APPROVAL:
        // TRANSLATORS: %s stands for full name of the peer
        m_infoLabel.Printf( _( "Awaiting approval from %s..." ), m_peerName );

        m_infoCancel->Show();
        break;

    case HistoryPendingState::OVERWRITE_NEEDED:
        m_infoLabel = _( "This request will overwrite one or more files!" );

        m_infoOverwrite->Show();
        m_infoCancel->Show();
        break;

    case HistoryPendingState::TRANSFER_PAUSED:
        m_infoPause->SetLabel( _( "R&esume" ) );

        m_infoProgress->Show();
        m_infoPause->Show();
        m_infoStop->Show();
        break;

    case HistoryPendingState::TRANSFER_RUNNING:
        m_infoPause->SetLabel( _( "&Pause" ) );

        m_infoProgress->Show();
        m_infoPause->Show();
        m_infoStop->Show();
        break;
    }

    calculateLayout();
}

void HistoryPendingElement::onPaint( wxPaintEvent& event )
{
    wxPaintDC dc( this );
    wxSize size = dc.GetSize();
    wxColour GRAY = wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT );

    // Draw op icon

    wxCoord iconOffset;
    wxCoord iconWidth;
    wxCoord contentOffsetX;

    if ( m_fileIcon.IsOk() )
    {
        iconOffset = ( size.GetHeight() - m_fileIcon.GetHeight() ) / 2;
        iconWidth = m_fileIcon.GetWidth();
        dc.DrawIcon( m_fileIcon, iconOffset, iconOffset );
    }
    else
    {
        const wxBitmap& icon = determineBitmapToDraw();
        iconOffset = ( size.GetHeight() - icon.GetHeight() ) / 2;
        iconWidth = icon.GetWidth();
        dc.DrawBitmap( icon, iconOffset, iconOffset );
    }

    contentOffsetX = iconOffset + iconWidth + FromDIP( 8 );

    // Draw direction badge

    const wxBitmap& badge = ( ( m_data.outcoming )
            ? m_bitmaps->badgeUp
            : m_bitmaps->badgeDown );
    wxCoord badgeOffset = iconOffset + iconWidth - badge.GetWidth();
    dc.DrawBitmap( badge, badgeOffset, badgeOffset );

    // Draw operation heading

    dc.SetFont( Utils::get()->getHeaderFont() );
    dc.SetTextForeground( GetForegroundColour() );

    int contentWidth = m_info->GetPosition().x - contentOffsetX;
    int offsetY = FromDIP( 6 );

    Utils::drawTextEllipse( dc, determineHeaderString(),
        wxPoint( contentOffsetX, offsetY ), contentWidth );

    offsetY += dc.GetTextExtent( "A" ).y + FromDIP( 4 );

    // Draw details labels

    dc.SetFont( GetFont() );
    dc.SetTextForeground( GRAY );

    const wxString sizeLabel = _( "Total size:" );
    const wxString startTimeLabel = _( "Start time:" );

    int sizeWidth = dc.GetTextExtent( sizeLabel ).x;
    int startTimeWidth = dc.GetTextExtent( startTimeLabel ).x;
    int columnWidth = std::max( sizeWidth, startTimeWidth );
    int lineHeight = dc.GetTextExtent( "A" ).y + FromDIP( 4 );

    dc.DrawText( sizeLabel,
        contentOffsetX + columnWidth - sizeWidth, offsetY );
    dc.DrawText( startTimeLabel,
        contentOffsetX + columnWidth - startTimeWidth, offsetY + lineHeight );

    dc.SetTextForeground( GetForegroundColour() );

    int detailsWidth = contentWidth - columnWidth - FromDIP( 4 );

    int detailsX = columnWidth + contentOffsetX + FromDIP( 4 );

    Utils::drawTextEllipse( dc, Utils::fileSizeToString( m_data.totalSizeBytes ),
        wxPoint( detailsX, offsetY ), detailsWidth );
    Utils::drawTextEllipse( dc,
        // TRANSLATORS: time format string
        Utils::formatDate( m_data.opStartTime, _( "%I:%M %p" ).ToStdString() ),
        wxPoint( detailsX, offsetY + lineHeight ), detailsWidth );

    // Draw status label

    int labelX, labelY;
    wxPoint buttonPos = m_buttonSizer->GetPosition();

    // We determine y-coord positon of the label
    labelY = buttonPos.y - m_infoSpacing;

    dc.SetFont( GetFont() );

    wxPoint sizerPos = m_info->GetPosition();
    wxSize sizerSize = m_info->GetSize();
    wxCoord labelWidth = dc.GetTextExtent( m_infoLabel ).x;

    labelX = sizerPos.x + sizerSize.x / 2 - labelWidth / 2;

    dc.SetTextForeground( GetForegroundColour() );
    dc.DrawText( m_infoLabel, labelX, labelY );

    event.Skip( true );
}

void HistoryPendingElement::onDpiChanged( wxDPIChangedEvent& event )
{
    if ( m_fileIconLoc.IsOk() )
    {
        m_fileIcon = Utils::extractIconWithSize(
            m_fileIconLoc, FromDIP( ICON_SIZE ) );
    }

    Refresh();
}

const wxBitmap& HistoryPendingElement::determineBitmapToDraw() const
{
    if ( m_data.numFolders == 0 )
    {
        if ( m_data.numFiles > 1 )
        {
            return m_bitmaps->transferFileFile;
        }

        return m_bitmaps->transferFileX;
    }

    if ( m_data.numFiles == 0 )
    {
        if ( m_data.numFolders > 1 )
        {
            return m_bitmaps->transferDirDir;
        }

        return m_bitmaps->transferDirX;
    }

    return m_bitmaps->transferDirFile;
}

wxString HistoryPendingElement::determineHeaderString() const
{
    if ( m_data.numFiles == 0 && m_data.numFolders == 0 )
    {
        return _( "Empty" );
    }

    if ( ( m_data.numFiles == 1 && m_data.numFolders == 0 )
        || ( m_data.numFiles == 0 && m_data.numFolders == 1 ) )
    {
        // If we send a single element, return its name

        return m_data.singleElementName;
    }

    wxString filePart = wxPLURAL( "%d file", "%d files", m_data.numFiles );
    filePart.Printf( filePart.Clone(), m_data.numFiles );
    wxString folderPart = wxPLURAL(
        "%d folder", "%d folders", m_data.numFolders );
    folderPart.Printf( folderPart.Clone(), m_data.numFolders );

    if ( m_data.numFiles == 0 )
    {
        return folderPart;
    }

    if ( m_data.numFolders == 0 )
    {
        return filePart;
    }

    wxString both;

    // TRANSLATORS: <2 folders> and <5 files>
    both.Printf( _( "%s and %s" ), folderPart, filePart );

    return both;
}

};
