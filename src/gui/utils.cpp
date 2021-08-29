#include "utils.hpp"

#ifdef _WIN32
#include <wx/msw/uxtheme.h>
#endif

namespace gui
{

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
                text.Left( center ) ).GetWidth();

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

};