#include "history_item_icon.hpp"

#include "utils.hpp"

#include <wx/filename.h>
#include <wx/mimetype.h>
#include <wx/translation.h>

namespace gui
{

const int HistoryIconItem::ICON_SIZE = 64;

HistoryIconItem::HistoryIconItem( wxWindow* parent, HistoryStdBitmaps* bmps )
    : HistoryItem( parent )
    , m_bitmaps( bmps )
    , m_singleElementName( wxEmptyString )
    , m_folderCount( 0 )
    , m_fileCount( 0 )
    , m_outcoming( false )
    , m_fileIcon( wxNullIcon )
    , m_fileIconLoc()
{
    // Events

    Bind( wxEVT_DPI_CHANGED, &HistoryIconItem::onDpiChanged, this );
}

void HistoryIconItem::setIcons( int folderCount, int fileCount,
    const wxString& fileExt )
{
    m_folderCount = folderCount;
    m_fileCount = fileCount;

    m_singleElementName = fileExt;

    m_fileIcon = wxNullIcon;
    m_fileIconLoc = wxIconLocation();

    if ( fileCount == 1 && folderCount == 0 )
    {
        // If this transfer is a single file, try to load its extension icon

        wxFileName fileName( fileExt );
        wxString extension = fileName.GetExt();

        wxIconLocation loc;
        wxFileType* fileType = wxTheMimeTypesManager->GetFileTypeFromExtension( extension );

        if ( fileType )
        {
            wxLogNull logNull;

            if ( fileType->GetIcon( &loc ) && wxFileExists( loc.GetFileName() ) )
            {
                if ( loc.GetFileName() != m_fileIconLoc.GetFileName()
                    || loc.GetIndex() != m_fileIconLoc.GetIndex() )
                {
                    m_fileIcon = Utils::extractIconWithSize(
                        loc, FromDIP( ICON_SIZE ) );
                    m_fileIconLoc = loc;
                }
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

    Refresh();
}

void HistoryIconItem::setOutcoming( bool outcoming )
{
    m_outcoming = outcoming;

    Refresh();
}

const wxBitmap& HistoryIconItem::determineBitmapToDraw() const
{
    if ( m_folderCount == 0 )
    {
        if ( m_fileCount > 1 )
        {
            return m_bitmaps->transferFileFile;
        }

        return m_bitmaps->transferFileX;
    }

    if ( m_fileCount == 0 )
    {
        if ( m_folderCount > 1 )
        {
            return m_bitmaps->transferDirDir;
        }

        return m_bitmaps->transferDirX;
    }

    return m_bitmaps->transferDirFile;
}

wxString HistoryIconItem::determineHeaderString() const
{
    if ( m_fileCount == 0 && m_folderCount == 0 )
    {
        return _( "Empty" );
    }

    if ( ( m_fileCount == 1 && m_folderCount == 0 )
        || ( m_fileCount == 0 && m_folderCount == 1 ) )
    {
        // If we send a single element, return its name

        return m_singleElementName;
    }

    wxString filePart = wxPLURAL( "%d file", "%d files", m_fileCount );
    filePart.Printf( filePart.Clone(), m_fileCount );
    wxString folderPart = wxPLURAL(
        "%d folder", "%d folders", m_folderCount );
    folderPart.Printf( folderPart.Clone(), m_folderCount );

    if ( m_fileCount == 0 )
    {
        return folderPart;
    }

    if ( m_folderCount == 0 )
    {
        return filePart;
    }

    wxString both;

    // TRANSLATORS: format string, e.g. <2 folders> and <5 files>
    both.Printf( _( "%s and %s" ), folderPart, filePart );

    return both;
}

wxCoord HistoryIconItem::drawIcon( wxPaintDC& dc )
{
    wxSize size = dc.GetSize();
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

    const wxBitmap& badge = ( ( m_outcoming )
            ? m_bitmaps->badgeUp
            : m_bitmaps->badgeDown );
    wxCoord badgeOffset = iconOffset + iconWidth - badge.GetWidth();
    dc.DrawBitmap( badge, badgeOffset, badgeOffset );

    return contentOffsetX;
}

void HistoryIconItem::onDpiChanged( wxDPIChangedEvent& event )
{
    if ( m_fileIconLoc.IsOk() )
    {
        m_fileIcon = Utils::extractIconWithSize(
            m_fileIconLoc, FromDIP( HistoryIconItem::ICON_SIZE ) );
    }

    Refresh();
}

};
