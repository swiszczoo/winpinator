#include "utils.hpp"

#ifdef _WIN32
#include <Shlobj.h>
#include <shellapi.h>
#include <wx/msw/private.h>
#include <wx/msw/uxtheme.h>
#endif

#include <chrono>
#include <iomanip>
#include <sstream>

namespace gui
{

const wxColour Utils::GREEN_ACCENT = wxColour( 30, 123, 30 );
const wxColour Utils::GREEN_BACKGROUND = wxColour( 152, 230, 152 );
const wxColour Utils::ORANGE_ACCENT = wxColour( 204, 102, 0 );
const wxColour Utils::ORANGE_BACKGROUND = wxColour( 255, 217, 179 );
const wxColour Utils::RED_ACCENT = wxColour( 204, 0, 0 );
const wxColour Utils::RED_BACKGROUND = wxColour( 255, 190, 190 );

std::unique_ptr<Utils> Utils::s_inst = nullptr;

Utils::Utils()
{
    m_headerFont = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );
    m_headerFont.SetPointSize( 13 );

#ifdef _WIN32
    if ( wxUxThemeIsActive() )
    {
        m_headerColor = wxColour( 0, 51, 153 );
    }
    else
    {
        m_headerColor = wxSystemSettings::GetColour( wxSYS_COLOUR_INACTIVECAPTIONTEXT );
    }
#else
    m_headerFont.MakeBold();
    m_headerColor = wxSystemSettings::GetColour( wxSYS_COLOUR_INACTIVECAPTIONTEXT );
#endif
}

Utils* Utils::get()
{
    if ( !Utils::s_inst )
    {
        Utils::s_inst = std::make_unique<Utils>();
    }

    return Utils::s_inst.get();
}

wxString Utils::makeIntResource( int resource )
{
    return wxString::Format( "#%d", resource );
}

const wxFont& Utils::getHeaderFont() const
{
    return m_headerFont;
}

wxColour Utils::getHeaderColor() const
{
    return m_headerColor;
}

void Utils::drawTextEllipse( wxDC& dc, const wxString& text,
    const wxPoint& pnt, const wxCoord maxWidth )
{
    const wxCoord ellipsisLength = dc.GetTextExtent( "..." ).GetWidth();

    if ( maxWidth < ellipsisLength )
    {
        return;
    }

    if ( dc.GetTextExtent( text ).GetWidth() <= maxWidth )
    {
        dc.DrawText( text, pnt );
    }
    else if ( text.length() > 0 )
    {
        // Perform binsearch to find optimal text length

        int p = 0;
        int k = text.length();

        while ( k - p > 1 )
        {
            int center = ( p + k ) / 2;

            const wxCoord currWidth = dc.GetTextExtent(
                                            text.Left( center ) )
                                          .GetWidth();

            if ( currWidth < maxWidth - ellipsisLength )
            {
                p = center;
            }
            else
            {
                k = center;
            }
        }

        const wxString text1 = text.Left( p );

        if ( dc.GetTextExtent( text1 ).GetWidth() <= maxWidth - ellipsisLength )
        {
            dc.DrawText( text1 + "...", pnt );
        }
        else
        {
            const wxString text2 = text.Left( p - 1 );
            dc.DrawText( text2 + "...", pnt );
        }
    }
}

wxString Utils::getStatusString( srv::RemoteStatus status )
{
    switch ( status )
    {
    case srv::RemoteStatus::OFFLINE:
        return _( "Offline" );
    case srv::RemoteStatus::REGISTRATION:
        return _( "Registration in progress..." );
    case srv::RemoteStatus::INIT_CONNECTING:
        return _( "Connecting..." );
    case srv::RemoteStatus::UNREACHABLE:
        return _( "Host unreachable" );
    case srv::RemoteStatus::AWAITING_DUPLEX:
        return _( "Establishing duplex connection..." );
    case srv::RemoteStatus::ONLINE:
        return _( "Ready" );
    }

    return _( "Unknown" );
};

wxIcon Utils::extractIconWithSize( const wxIconLocation& loc, wxCoord dim )
{
    HICON largeIcon;
    if ( SHDefExtractIconW( loc.GetFileName().wc_str(),
             loc.GetIndex(), 0, &largeIcon, NULL, dim )
        != S_OK )
    {
        return wxIcon( loc );
    }

    SIZE test;
    getIconDimensions( largeIcon, &test );

    wxIcon icon;
    icon.CreateFromHICON( largeIcon );

    return icon;
}

bool Utils::getIconDimensions( HICON hico, SIZE* psiz )
{
    ICONINFO ii;
    BOOL fResult = GetIconInfo( hico, &ii );
    if ( fResult )
    {
        BITMAP bm;
        fResult = GetObject( ii.hbmMask, sizeof( bm ), &bm ) == sizeof( bm );
        if ( fResult )
        {
            psiz->cx = bm.bmWidth;
            psiz->cy = ii.hbmColor ? bm.bmHeight : bm.bmHeight / 2;
        }
        if ( ii.hbmMask )
            DeleteObject( ii.hbmMask );
        if ( ii.hbmColor )
            DeleteObject( ii.hbmColor );
    }
    return fResult;
}

wxString Utils::fileSizeToString( long long bytes )
{
    wxString out;

    if ( bytes > 1024 * 1024 * 1024 )
    {
        out.Printf( "%.1lf GB", bytes / 1024.0 / 1024.0 / 1024.0 );
    }
    else if ( bytes > 1024 * 1024 )
    {
        out.Printf( "%.1lf MB", bytes / 1024.0 / 1024.0 );
    }
    else if ( bytes > 1024 )
    {
        out.Printf( "%.1lf KB", bytes / 1024.0 );
    }
    else
    {
        out.Printf( "%lld B", bytes );
    }

    return out;
}

wxString Utils::formatDate( uint64_t timestamp, std::string format )
{
    std::time_t temp = timestamp;
    std::tm* t = std::localtime( &temp );
    std::stringstream ss;
    ss << std::put_time( t, format.c_str() );
    std::string output = ss.str();

    return output;
}

};