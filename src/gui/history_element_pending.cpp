#include "history_element_pending.hpp"
#include "utils.hpp"

#include <wx/filename.h>
#include <wx/mimetype.h>

namespace gui
{

const int HistoryPendingElement::ICON_SIZE = 64;
const int HistoryPendingElement::PROGRESS_RANGE = 1000000;

HistoryPendingElement::HistoryPendingElement( wxWindow* parent,
    HistoryStdBitmaps* bitmaps )
    : HistoryIconItem( parent, bitmaps )
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
{
    wxBoxSizer* horzSizer = new wxBoxSizer( wxHORIZONTAL );

    horzSizer->AddStretchSpacer( 3 );

    // Await peer approval
    m_info = new wxBoxSizer( wxVERTICAL );
    horzSizer->Add( m_info, 2, wxEXPAND | wxLEFT | wxRIGHT, FromDIP( 8 ) );

    m_info->AddStretchSpacer( 1 );

    m_infoProgress = new wxGauge( this, wxID_ANY, PROGRESS_RANGE );
    m_infoProgress->SetMinSize( FromDIP( wxSize( 16, 16 ) ) );
    m_infoProgress->Hide();
    m_info->Add( m_infoProgress, 0, wxEXPAND | wxBOTTOM, FromDIP( 3 ) );

    wxFont guiFont = GetFont();
    int hejt = guiFont.GetPixelSize().GetHeight();
    m_infoSpacing = guiFont.GetPixelSize().GetHeight() * 1.2 + FromDIP( 6 );
    m_info->AddSpacer( m_infoSpacing );

    m_buttonSizer = new wxBoxSizer( wxHORIZONTAL );

    m_infoCancel = new wxButton( this, wxID_ANY, _( "&Cancel" ) );
    m_buttonSizer->Add( m_infoCancel, 0, wxEXPAND | wxRIGHT, FromDIP( 2 ) );

    m_infoAllow = new wxButton( this, wxID_ANY, _( "&Accept" ) );
    m_buttonSizer->Add( m_infoAllow, 0, wxEXPAND | wxRIGHT, FromDIP( 2 ) );

    m_infoReject = new wxButton( this, wxID_ANY, _( "&Decline" ) );
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
}

void HistoryPendingElement::setData( const HistoryPendingData& newData )
{
    m_data = newData;

    setIcons( newData.numFolders, newData.numFiles, newData.singleElementName );
    setOutcoming( newData.outcoming );
    setFinished( false );

    if ( newData.opState == HistoryPendingState::TRANSFER_PAUSED
        || newData.opState == HistoryPendingState::TRANSFER_RUNNING )
    {
        updateProgress( newData.sentBytes );
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

void HistoryPendingElement::updateProgress( int sentBytes )
{
    m_data.sentBytes = sentBytes;

    int remainingSecs = calculateRemainingSeconds();
    int bytesPerSecond = calculateTransferSpeed();

    wxString remainingString;
    wxString speedString;

    if ( remainingSecs == -1 )
    {
        remainingString = _( "calculating remaining time" );
    }
    else if ( remainingSecs < 5 )
    {
        remainingString = _( "a few seconds remaining" );
    }
    else if ( remainingSecs < 60 ) // Less than a minute
    {
        remainingString.Printf(
            wxPLURAL( "%d sec remaining", "%d secs remaining", remainingSecs ),
            remainingSecs );
    }
    else if ( remainingSecs < 60 * 60 ) // Less than an hour
    {
        int minutes = (int)round( remainingSecs / 60.f );

        remainingString.Printf(
            wxPLURAL( "%d min remaining", "%d mins remaining", minutes ),
            minutes );
    }
    else if ( remainingSecs < 24 * 60 * 60 ) // Less than a day
    {
        int hours = (int)round( remainingSecs / ( 3600.f ) );

        remainingString.Printf(
            wxPLURAL( "%d hour remaining", "%d hours remaining", hours ),
            hours );
    }
    else if ( remainingSecs < 7 * 24 * 60 * 60 ) // Less than a week
    {
        int days = (int)round( remainingSecs / ( 3600.f ) );

        remainingString.Printf(
            wxPLURAL( "%d day remaining", "%d days remaining", days ),
            days );
    }
    else
    {
        remainingString = _( "many days remaining" );
    }

    // TRANSLATORS: this is a format string for transfer speed,
    // the %s part will be replaced with appropriate file size equivalent,
    // e.g. 25,4MB or 32,6KB
    speedString.Printf( _( "%s/s" ), Utils::fileSizeToString( bytesPerSecond ) );

    wxString detailsString;

    // TRANSLATORS: the subsequent %s placeholders stand for:
    // current sent bytes, transfer total size, transfer speed, remaining time
    detailsString.Printf( _( L"%s of %s \x2022 %s \x2022 %s" ),
        Utils::fileSizeToString( m_data.sentBytes ),
        Utils::fileSizeToString( m_data.totalSizeBytes ),
        speedString, remainingString );

    m_infoLabel = detailsString;

    m_infoProgress->SetValue( (int)( (float)m_data.sentBytes
        / (float)m_data.totalSizeBytes * PROGRESS_RANGE ) );

    Refresh();
}

void HistoryPendingElement::calculateLayout()
{
    wxCoord width = 0;

    if ( m_data.opState == HistoryPendingState::TRANSFER_PAUSED
        || m_data.opState == HistoryPendingState::TRANSFER_RUNNING )
    {
        // TRANSLATORS: This string does not show up in app, but is used
        // to determine progress bar width, so it should be longest possible
        // transfer progress label text
        width = GetTextExtent(
            _( L"999.9MB of 999.9MB \x2022 999.9MB/s \x2022 a few seconds remaining" ) )
                    .x;
    }
    else
    {
        width = GetTextExtent( m_infoLabel ).x;
    }

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

    case HistoryPendingState::DISK_FULL:
        m_infoLabel = _( "Not enough disk space! Files cannot be received!" );

        m_infoCancel->Show();
        break;
    }

    calculateLayout();
}

void HistoryPendingElement::onPaint( wxPaintEvent& event )
{
    wxPaintDC dc( this );
    wxSize size = dc.GetSize();
    wxColour GRAY = wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT );

    wxCoord contentOffsetX = drawIcon( dc );

    // Draw operation heading

    dc.SetFont( Utils::get()->getHeaderFont() );
    dc.SetTextForeground( GetForegroundColour() );

    int contentWidth = m_info->GetPosition().x - contentOffsetX;
    int offsetY = FromDIP( 6 );

    Utils::drawTextEllipse( dc, determineHeaderString(),
        wxPoint( contentOffsetX, offsetY ), contentWidth );

    offsetY += dc.GetTextExtent( "A" ).y + 4;

    // Draw details labels

    dc.SetFont( GetFont() );
    dc.SetTextForeground( GRAY );

    const wxString typeLabel = _( "Type:" );
    const wxString sizeLabel = _( "Total size:" );

    int typeWidth = dc.GetTextExtent( typeLabel ).x;
    int sizeWidth = dc.GetTextExtent( sizeLabel ).x;
    int columnWidth = std::max( sizeWidth, typeWidth );
    int lineHeight = dc.GetTextExtent( "A" ).y + 4;

    int detailsWidth = contentWidth - columnWidth - FromDIP( 4 );

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
            m_info->GetPosition().x - FromDIP( 4 ) - typeX );

        Utils::drawTextEllipse( dc, 
            Utils::fileSizeToString( m_data.totalSizeBytes ),
            wxPoint( sizeX, offsetY + lineHeight ),
            m_info->GetPosition().x - FromDIP( 4 ) - typeX );
    }

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

int HistoryPendingElement::calculateRemainingSeconds() const
{
    return 200;
}

int HistoryPendingElement::calculateTransferSpeed() const
{
    return 1e6;
}

};
