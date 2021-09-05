#include "history_item.hpp"

#include <wx/msw/uxtheme.h>

namespace gui
{

HistoryItem::HistoryItem( wxWindow* parent )
    : wxPanel( parent )
    , m_hovering( false )
{
    SetWindowTheme( GetHWND(), _T("Explorer"), NULL );

    refreshHoverState();

    // Events

    Bind( wxEVT_ERASE_BACKGROUND, &HistoryItem::onErase, this );
}

void HistoryItem::eraseNullSink( wxEraseEvent& event )
{
    // This is a null sink/handler, it does completely nothing
    // but blocks event propagation
}

void HistoryItem::updateHoverState( bool insideParent )
{
    bool previousState = m_hovering;

    if ( !IsShown() )
    {
        // If window is hidden, don't waste time making calculations
        m_hovering = false;
        return;
    }

    if ( insideParent )
    {
        refreshHoverState();
    }
    else
    {
        m_hovering = false;
    }

    if ( m_hovering != previousState )
    {
        Refresh();
    }
}

void HistoryItem::refreshHoverState()
{
    wxPoint mousePos = wxGetMousePosition();
    wxRect itemRct = GetScreenRect();
    wxRect parentRct = GetParent()->GetScreenRect();

    if ( itemRct.Contains( mousePos ) && parentRct.Contains( mousePos ) )
    {
        m_hovering = true;
    }
    else
    {
        m_hovering = false;
    }
}

void HistoryItem::onErase( wxEraseEvent& event )
{
    wxDC* dc = event.GetDC();
    wxRect rect = wxRect( dc->GetSize() );

    if ( wxUxThemeIsActive() )
    {
        wxUxThemeHandle theme( this, L"LISTVIEW" );

        int partId = LVP_LISTITEM;
        int stateId = m_hovering ? LISS_HOT : LISS_NORMAL;

        WXHDC winDc = dc->GetHDC();
        RECT dcRect;
        dcRect.left = rect.GetX();
        dcRect.top = rect.GetY();
        dcRect.right = rect.GetRight();
        dcRect.bottom = rect.GetBottom();

        DoEraseBackground( winDc );

        if ( stateId != LISS_NORMAL && stateId != LISS_DISABLED )
        {
            DrawThemeBackground( theme, winDc, partId, stateId, &dcRect, NULL );
        }
        else
        {
            event.Skip( true );
        }
    }
    else
    {
        event.Skip( true );
    }
}

};
