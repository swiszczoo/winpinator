#include "tool_button.hpp"

#include <wx/generic/private/markuptext.h>
#include <wx/msw/anybutton.h>
#include <wx/msw/uxtheme.h>
#include <wx/msw/private/dc.h>



namespace
{

const int XP_BUTTON_EXTRA_MARGIN = 1;
const int OD_BUTTON_MARGIN = 4;

// return the button state using both the ODS_XXX flags specified in state
// parameter and the current button state
wxAnyButton::State GetButtonState( wxAnyButton* btn, UINT state )
{
    if ( state & ODS_DISABLED )
        return wxAnyButton::State_Disabled;

    // We need to check for the pressed state of the button itself before the
    // other checks because even if it is selected or current, it it still
    // pressed first and foremost.
    const wxAnyButton::State btnState = btn->GetNormalState();

    if ( btnState == wxAnyButton::State_Pressed || state & ODS_SELECTED )
        return wxAnyButton::State_Pressed;

    if ( btn->HasCapture() || btn->IsMouseInWindow() )
        return wxAnyButton::State_Current;

    if ( state & ODS_FOCUS )
        return wxAnyButton::State_Focused;

    return btnState;
}

void DrawButtonText( HDC hdc,
    RECT* pRect,
    wxAnyButton* btn,
    int flags )
{
    const wxString text = btn->GetLabel();

    // To get a native look for owner-drawn button in disabled state (without
    // theming) we must use DrawState() to draw the text label.
    //
    // Notice that we use the enabled state at MSW, not wx, level because we
    // don't want to grey it out when it's disabled just because its parent is
    // disabled by MSW as it happens when showing a modal dialog, but we do
    // want to grey it out if either it or its parent are explicitly disabled
    // at wx level, see #18011.
    if ( !wxUxThemeIsActive() && !::IsWindowEnabled( GetHwndOf( btn ) ) )
    {
        // However using DrawState() has some drawbacks:
        // 1. It generally doesn't support alignment flags (except right
        //    alignment), so we need to align the text on our own.
        // 2. It doesn't support multliline texts and there is necessary to
        //    draw/align multiline text line by line.

        // Compute bounding rect for the whole text.
        RECT rc;
        ::SetRectEmpty( &rc );
        ::DrawText( hdc, text.t_str(), text.length(), &rc, DT_CALCRECT );

        const LONG h = rc.bottom - rc.top;

        // Based on wxButton flags determine bottom edge of the drawing rect
        // inside the entire button area.
        int y0;
        if ( btn->HasFlag( wxBU_BOTTOM ) )
        {
            y0 = pRect->bottom - h;
        }
        else if ( !btn->HasFlag( wxBU_TOP ) )
        {
            // DT_VCENTER
            y0 = pRect->top + ( pRect->bottom - pRect->top ) / 2 - h / 2;
        }
        else // DT_TOP is the default
        {
            y0 = pRect->top;
        }

        UINT dsFlags = DSS_DISABLED;
        if ( flags & DT_HIDEPREFIX )
            dsFlags |= ( DSS_HIDEPREFIX | DST_PREFIXTEXT );
        else
            dsFlags |= DST_TEXT;

        const wxArrayString lines = wxSplit( text, '\n', '\0' );
        const int hLine = h / lines.size();
        for ( size_t lineNum = 0; lineNum < lines.size(); lineNum++ )
        {
            // Each line must be aligned in horizontal direction individually.
            ::SetRectEmpty( &rc );
            ::DrawText( hdc, lines[lineNum].t_str(), lines[lineNum].length(),
                &rc, DT_CALCRECT );
            const LONG w = rc.right - rc.left;

            // Based on wxButton flags set horizontal position of the rect
            // inside the entire button area. Text is always centered for
            // multiline label.
            if ( ( !btn->HasFlag( wxBU_LEFT ) && !btn->HasFlag( wxBU_RIGHT ) ) || lines.size() > 1 )
            {
                // DT_CENTER
                rc.left = pRect->left + ( pRect->right - pRect->left ) / 2 - w / 2;
                rc.right = rc.left + w;
            }
            else if ( btn->HasFlag( wxBU_RIGHT ) )
            {
                rc.right = pRect->right;
                rc.left = rc.right - w;
            }
            else // DT_LEFT is the default
            {
                rc.left = pRect->left;
                rc.right = rc.left + w;
            }

            ::OffsetRect( &rc, 0, y0 + lineNum * hLine );

            ::DrawState( hdc, NULL, NULL, wxMSW_CONV_LPARAM( lines[lineNum] ),
                lines[lineNum].length(),
                rc.left, rc.top, rc.right, rc.bottom, dsFlags );
        }
    }
    else // Button is enabled or using themes.
    {
        if ( text.find( wxT( '\n' ) ) != wxString::npos )
        {
            // draw multiline label

            // first we need to compute its bounding rect
            RECT rc;
            ::CopyRect( &rc, pRect );
            ::DrawText( hdc, text.t_str(), text.length(), &rc,
                flags | DT_CALCRECT );

            // now position this rect inside the entire button area: notice
            // that DrawText() doesn't respect alignment flags for multiline
            // text, which is why we have to do it on our own (but still use
            // the horizontal alignment flags for the individual lines to be
            // aligned correctly)
            const LONG w = rc.right - rc.left;
            const LONG h = rc.bottom - rc.top;

            if ( btn->HasFlag( wxBU_RIGHT ) )
            {
                rc.left = pRect->right - w;

                flags |= DT_RIGHT;
            }
            else if ( !btn->HasFlag( wxBU_LEFT ) )
            {
                rc.left = pRect->left + ( pRect->right - pRect->left ) / 2 - w / 2;

                flags |= DT_CENTER;
            }
            else // wxBU_LEFT
            {
                rc.left = pRect->left;
            }

            if ( btn->HasFlag( wxBU_BOTTOM ) )
            {
                rc.top = pRect->bottom - h;
            }
            else if ( !btn->HasFlag( wxBU_TOP ) )
            {
                rc.top = pRect->top + ( pRect->bottom - pRect->top ) / 2 - h / 2;
            }
            else // wxBU_TOP
            {
                rc.top = pRect->top;
            }

            rc.right = rc.left + w;
            rc.bottom = rc.top + h;

            ::DrawText( hdc, text.t_str(), text.length(), &rc, flags );
        }
        else // single line label
        {
            // translate wx button flags to alignment flags for DrawText()
            if ( btn->HasFlag( wxBU_RIGHT ) )
            {
                flags |= DT_RIGHT;
            }
            else if ( !btn->HasFlag( wxBU_LEFT ) )
            {
                flags |= DT_CENTER;
            }
            //else: DT_LEFT is the default anyhow (and its value is 0 too)

            if ( btn->HasFlag( wxBU_BOTTOM ) )
            {
                flags |= DT_BOTTOM;
            }
            else if ( !btn->HasFlag( wxBU_TOP ) )
            {
                flags |= DT_VCENTER;
            }
            //else: as above, DT_TOP is the default

            // notice that we must have DT_SINGLELINE for vertical alignment
            // flags to work
            ::DrawText( hdc, text.t_str(), text.length(), pRect,
                flags | DT_SINGLELINE );
        }
    }
}

void DrawRect( HDC hdc, const RECT& r, COLORREF color )
{
    wxDrawHVLine( hdc, r.left, r.top, r.right, r.top, color, 1 );
    wxDrawHVLine( hdc, r.right, r.top, r.right, r.bottom, color, 1 );
    wxDrawHVLine( hdc, r.right, r.bottom, r.left, r.bottom, color, 1 );
    wxDrawHVLine( hdc, r.left, r.bottom, r.left, r.top, color, 1 );
}

void DrawButtonFrame( HDC hdc, RECT& rectBtn,
    bool selected, bool pushed )
{
    RECT r;
    ::CopyRect( &r, &rectBtn );

    COLORREF clrBlack = ::GetSysColor( COLOR_3DDKSHADOW ),
             clrGrey = ::GetSysColor( COLOR_3DSHADOW ),
             clrLightGr = ::GetSysColor( COLOR_3DLIGHT ),
             clrWhite = ::GetSysColor( COLOR_3DHILIGHT );

    r.right--;
    r.bottom--;

    if ( pushed )
    {
        DrawRect( hdc, r, clrBlack );

        ::InflateRect( &r, -1, -1 );

        DrawRect( hdc, r, clrGrey );
    }
    else // !pushed
    {
        if ( selected )
        {
            DrawRect( hdc, r, clrBlack );

            ::InflateRect( &r, -1, -1 );
        }

        wxDrawHVLine( hdc, r.left, r.bottom, r.right, r.bottom, clrBlack, 1 );
        wxDrawHVLine( hdc, r.right, r.bottom, r.right, r.top - 1, clrBlack, 1 );

        wxDrawHVLine( hdc, r.left, r.bottom - 1, r.left, r.top, clrWhite, 1 );
        wxDrawHVLine( hdc, r.left, r.top, r.right, r.top, clrWhite, 1 );

        wxDrawHVLine( hdc, r.left + 1, r.bottom - 2, r.left + 1, r.top + 1, clrLightGr, 1 );
        wxDrawHVLine( hdc, r.left + 1, r.top + 1, r.right - 1, r.top + 1, clrLightGr, 1 );

        wxDrawHVLine( hdc, r.left + 1, r.bottom - 1, r.right - 1, r.bottom - 1, clrGrey, 1 );
        wxDrawHVLine( hdc, r.right - 1, r.bottom - 1, r.right - 1, r.top, clrGrey, 1 );
    }

    InflateRect( &rectBtn, -OD_BUTTON_MARGIN, -OD_BUTTON_MARGIN );
}

#if wxUSE_UXTHEME
void DrawXPBackground( wxAnyButton* button, HDC hdc, RECT& rectBtn, UINT state )
{
    wxUxThemeHandle theme( button, L"BUTTON" );

    // this array is indexed by wxAnyButton::State values and so must be kept in
    // sync with it
    static const int uxStates[] = {
        CMDLS_NORMAL, CMDLS_HOT, CMDLS_PRESSED, CMDLS_DISABLED, CMDLS_DEFAULTED
    };

    int iState = uxStates[GetButtonState( button, state )];

    // draw parent background if needed
    /*
    if ( ::IsThemeBackgroundPartiallyTransparent(
             theme,
             BP_PUSHBUTTON,
             iState ) )
    {
        // Set this button as the one whose background is being erased: this
        // allows our WM_ERASEBKGND handler used by DrawThemeParentBackground()
        // to correctly align the background brush with this window instead of
        // the parent window to which WM_ERASEBKGND is sent. Notice that this
        // doesn't work with custom user-defined EVT_ERASE_BACKGROUND handlers
        // as they won't be aligned but unfortunately all the attempts to fix
        // it by shifting DC origin before calling DrawThemeParentBackground()
        // failed to work so we at least do this, even though this is far from
        // being the perfect solution.
        wxWindowBeingErased = button;

        ::DrawThemeParentBackground( GetHwndOf( button ), hdc, &rectBtn );

        wxWindowBeingErased = NULL;
    }
    */

    // draw background
    ::DrawThemeBackground( theme, hdc, BP_COMMANDLINK, iState,
        &rectBtn, NULL );

    // calculate content area margins, using the defaults in case we fail to
    // retrieve the current theme margins
    MARGINS margins = { 3, 3, 3, 3 };
    ::GetThemeMargins( theme, hdc, BP_COMMANDLINK, iState,
        TMT_CONTENTMARGINS, &rectBtn, &margins );
    ::InflateRect( &rectBtn, -margins.cxLeftWidth, -margins.cyTopHeight );

    if ( button->UseBgCol() && iState != CMDLS_HOT )
    {
        COLORREF colBg = wxColourToRGB( button->GetBackgroundColour() );
        AutoHBRUSH hbrushBackground( colBg );

        FillRect( hdc, &rectBtn, hbrushBackground );
    }

    ::InflateRect( &rectBtn, -XP_BUTTON_EXTRA_MARGIN, -XP_BUTTON_EXTRA_MARGIN );
}
#endif // wxUSE_UXTHEME

};

namespace gui
{

class wxButtonImageData : public wxObject
{
public:
    wxButtonImageData() { }
    virtual ~wxButtonImageData() { }

    virtual wxBitmap GetBitmap( wxAnyButton::State which ) const = 0;
    virtual void SetBitmap( const wxBitmap& bitmap, wxAnyButton::State which ) = 0;

    virtual wxSize GetBitmapMargins() const = 0;
    virtual void SetBitmapMargins( wxCoord x, wxCoord y ) = 0;

    virtual wxDirection GetBitmapPosition() const = 0;
    virtual void SetBitmapPosition( wxDirection dir ) = 0;
};

class wxMarkupText : public ::wxMarkupText
{
};

ToolButton::ToolButton( wxWindow* parent, wxWindowID id, const wxString& label, 
    const wxPoint& pos, const wxSize& size, long style, 
    const wxValidator& validator, const wxString& name )
    : wxButton()
    , m_hovering( false )
    , m_pressed( false )
    , m_focus( false )
{
    Create( parent, id, label, pos, size,
        style | wxBORDER_NONE, validator, name );

    MakeOwnerDrawn();

    // Events

    Bind( wxEVT_PAINT, &ToolButton::onPaint, this );
}

void ToolButton::onPaint( wxPaintEvent& event )
{
    wxPaintDC dc( this );

    dc.SetBrush( *wxWHITE_BRUSH );
    dc.SetPen( *wxTRANSPARENT_PEN );
    dc.DrawRectangle( wxPoint( 0, 0 ), dc.GetSize() );

    event.Skip();
}


//
// Code below is a slightly modified version of src/msw/anybutton.cpp


bool ToolButton::MSWOnDraw( WXDRAWITEMSTRUCT* wxdis )
{
    LPDRAWITEMSTRUCT lpDIS = (LPDRAWITEMSTRUCT)wxdis;
    HDC hdc = lpDIS->hDC;
    // We expect here a DC with default settings (in GM_COMPATIBLE mode
    // with non-scaled coordinates system) but will check this because
    // our line drawing function wxDrawHVLine() works effectively only
    // on a non-transformed DC.

    UINT state = lpDIS->itemState;
    switch ( GetButtonState( this, state ) )
    {
    case State_Disabled:
        state |= ODS_DISABLED;
        break;
    case State_Pressed:
        state |= ODS_SELECTED;
        break;
    case State_Focused:
        state |= ODS_FOCUS;
        break;
    default:
        break;
    }

    bool pushed = MSWIsPushed();

    RECT rectBtn;
    CopyRect( &rectBtn, &lpDIS->rcItem );

    // draw the button background
    if ( !HasFlag( wxBORDER_NONE ) )
    {
#if wxUSE_UXTHEME
        if ( wxUxThemeIsActive() )
        {
            DrawXPBackground( this, hdc, rectBtn, state );
        }
        else
#endif // wxUSE_UXTHEME
        {
            COLORREF colBg = wxColourToRGB( GetBackgroundColour() );

            // first, draw the background
            AutoHBRUSH hbrushBackground( colBg );
            FillRect( hdc, &rectBtn, hbrushBackground );

            // draw the border for the current state
            bool selected = ( state & ODS_SELECTED ) != 0;
            if ( !selected )
            {
                wxTopLevelWindow*
                    tlw
                    = wxDynamicCast( wxGetTopLevelParent( this ), wxTopLevelWindow );
                if ( tlw )
                {
                    selected = tlw->GetDefaultItem() == this;
                }
            }

            DrawButtonFrame( hdc, rectBtn, selected, pushed );
        }

        // draw the focus rectangle if we need it
        if ( ( state & ODS_FOCUS ) && !( state & ODS_NOFOCUSRECT ) )
        {
            CopyRect( &rectBtn, &lpDIS->rcItem );
            DrawFocusRect( hdc, &rectBtn );

#if wxUSE_UXTHEME
            if ( !wxUxThemeIsActive() )
#endif // wxUSE_UXTHEME
            {
                if ( pushed )
                {
                    // the label is shifted by 1 pixel to create "pushed" effect
                    OffsetRect( &rectBtn, 1, 1 );
                }
            }
        }
    }
    else
    {
        // clear the background (and erase any previous bitmap)
        COLORREF colBg = wxColourToRGB( GetBackgroundColour() );
        AutoHBRUSH hbrushBackground( colBg );
        FillRect( hdc, &rectBtn, hbrushBackground );
    }

    // draw the image, if any
    if ( m_imageData )
    {
        //
        // This is a very dirty solution. It might not work if the wxWidgets
        // library and this part of code were compiled by different compilers.
        //

        gui::wxButtonImageData* imageData = (gui::wxButtonImageData*)m_imageData;
        wxBitmap bmp = imageData->GetBitmap( GetButtonState( this, state ) );
        if ( !bmp.IsOk() )
            bmp = imageData->GetBitmap( State_Normal );

        const wxSize sizeBmp = bmp.GetSize();
        const wxSize margin = imageData->GetBitmapMargins();
        const wxSize sizeBmpWithMargins( sizeBmp + 2 * margin );
        wxRect rectButton( wxRectFromRECT( rectBtn ) );

        // for simplicity, we start with centred rectangle and then move it to
        // the appropriate edge
        wxRect rectBitmap = wxRect( sizeBmp ).CentreIn( rectButton );

        // move bitmap only if we have a label, otherwise keep it centered
        if ( ShowsLabel() )
        {
            switch ( imageData->GetBitmapPosition() )
            {
            default:
                wxFAIL_MSG( "invalid direction" );
                wxFALLTHROUGH;

            case wxLEFT:
                rectBitmap.x = rectButton.x + margin.x;
                rectButton.x += sizeBmpWithMargins.x;
                rectButton.width -= sizeBmpWithMargins.x;
                break;

            case wxRIGHT:
                rectBitmap.x = rectButton.GetRight() - sizeBmp.x - margin.x;
                rectButton.width -= sizeBmpWithMargins.x;
                break;

            case wxTOP:
                rectBitmap.y = rectButton.y + margin.y;
                rectButton.y += sizeBmpWithMargins.y;
                rectButton.height -= sizeBmpWithMargins.y;
                break;

            case wxBOTTOM:
                rectBitmap.y = rectButton.GetBottom() - sizeBmp.y - margin.y;
                rectButton.height -= sizeBmpWithMargins.y;
                break;
            }
        }

        wxDCTemp dst( (WXHDC)hdc );
        dst.DrawBitmap( bmp, rectBitmap.GetPosition(), true );

        wxCopyRectToRECT( rectButton, rectBtn );
    }

    // finally draw the label
    if ( ShowsLabel() )
    {
        COLORREF colFg = state & ODS_DISABLED
            ? ::GetSysColor( COLOR_GRAYTEXT )
            : wxColourToRGB( GetForegroundColour() );

        wxMSWImpl::wxTextColoursChanger changeFg( hdc, colFg, CLR_INVALID );
        wxMSWImpl::wxBkModeChanger changeBkMode( hdc, wxBRUSHSTYLE_TRANSPARENT );

#if wxUSE_MARKUP
        if ( m_markupText )
        {
            wxDCTemp dc( (WXHDC)hdc );
            dc.SetTextForeground( wxColour( colFg ) );
            dc.SetFont( GetFont() );

            m_markupText->Render( dc, wxRectFromRECT( rectBtn ),
                state & ODS_NOACCEL
                    ? wxMarkupText::Render_Default
                    : wxMarkupText::Render_ShowAccels );
        }
        else // Plain text label
#endif // wxUSE_MARKUP
        {
            // notice that DT_HIDEPREFIX doesn't work on old (pre-Windows 2000)
            // systems but by happy coincidence ODS_NOACCEL is not used under
            // them neither so DT_HIDEPREFIX should never be used there
            DrawButtonText( hdc, &rectBtn, this,
                state & ODS_NOACCEL ? DT_HIDEPREFIX : 0 );
        }
    }

    return true;
}

};
