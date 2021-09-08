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
    , m_infoLabel( nullptr )
    , m_infoCancel( nullptr )
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

    m_infoLabel = new wxStaticText( this, wxID_ANY, wxEmptyString );
    m_info->Add( m_infoLabel, 0, 
        wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, FromDIP( 6 ) );

    m_infoProgress = new wxGauge( this, wxID_ANY, 1000000 );
    m_infoProgress->SetValue( 500000 );
    m_infoProgress->SetMinSize( FromDIP( wxSize( 16, 16 ) ) );
    m_info->Add( m_infoProgress, 0, wxEXPAND | wxBOTTOM, FromDIP( 6 ) );

    m_infoCancel = new wxButton( this, wxID_ANY, _( "Cancel" ) );
    m_info->Add( m_infoCancel, 0, wxALIGN_CENTER_HORIZONTAL );
    
    m_info->AddStretchSpacer( 1 );


    SetSizer( horzSizer );
    SetMinSize( FromDIP( wxSize( 16, 76 ) ) );

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

    wxString page0LabelText;
    page0LabelText.Printf( _( "Awaiting approval from %s..." ), m_peerName );
    m_infoLabel->SetLabel( page0LabelText );
}

const wxString& HistoryPendingElement::getPeerName() const
{
    return m_peerName;
}

void HistoryPendingElement::onPaint( wxPaintEvent& event )
{
    wxPaintDC dc( this );
    wxSize size = dc.GetSize();

    // Draw op icon

    wxCoord iconOffset;
    wxCoord iconWidth;

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

    // Draw direction badge

    const wxBitmap& badge = ( ( m_data.outcoming )
            ? m_bitmaps->badgeUp
            : m_bitmaps->badgeDown );
    wxCoord badgeOffset = iconOffset + iconWidth - badge.GetWidth();
    dc.DrawBitmap( badge, badgeOffset, badgeOffset );

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

};
