#include "about_dialog.hpp"

#include "../../win32/resource.h"
#include "utils.hpp"

#include <wx/filename.h>
#include <wx/notebook.h>
#include <wx/html/htmlwin.h>
#include <wx/richtext/richtextctrl.h>
#include <wx/stdpaths.h>

namespace gui
{

AboutDialog::AboutDialog( wxWindow* parent )
    : wxDialog( parent, wxID_ANY, _( "About Winpinator..." ) )
    , m_banner( nullptr )
    , m_bannerBmp( wxNullBitmap )
{
    SetMinClientSize( FromDIP( wxSize( 500, 200 ) ) );

    m_versionFont = GetFont();
    m_versionFont.SetPointSize( 10 );

    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

    m_banner = new wxPanel( this );
    loadBitmap();
    sizer->Add( m_banner, 0, wxEXPAND | wxBOTTOM, FromDIP( 16 ) );

    wxNotebook* nbook = new wxNotebook( this, wxID_ANY );
    nbook->SetMinSize( FromDIP( wxSize( 0, 300 ) ) );
    sizer->Add( nbook, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP( 10 ) );

    wxSizer* btnSizer = CreateSeparatedButtonSizer( wxOK );
    sizer->Add( btnSizer, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 8 ) );
    sizer->AddSpacer( FromDIP( 10 ) );

    createAboutPage( nbook );
    createLicensePage( nbook );

    SetSizerAndFit( sizer );
    CenterOnParent();

    // Events
    Bind( wxEVT_DPI_CHANGED, &AboutDialog::onDpiChanged, this );
    m_banner->Bind( wxEVT_PAINT, &AboutDialog::onPaintBitmap, this );
}

void AboutDialog::loadBitmap()
{
    wxBitmap original;
    original.LoadFile( Utils::makeIntResource( IDB_ABOUT_BANNER ),
        wxBITMAP_TYPE_PNG_RESOURCE );

    wxImage toScale = original.ConvertToImage();

    int width = FromDIP( 500 );
    int height = FromDIP( 200 );

    m_bannerBmp = toScale.Scale( width, height, wxIMAGE_QUALITY_BICUBIC );
    m_banner->SetMinSize( m_bannerBmp.GetSize() );
    m_banner->SetSize( m_bannerBmp.GetSize() );
    m_banner->Refresh();
}

void AboutDialog::createAboutPage( wxNotebook* nb )
{
    wxPanel* panel = new wxPanel( nb );
    nb->AddPage( panel, _( "About" ), true );

    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
    wxRichTextCtrl* content = new wxRichTextCtrl( panel );
    content->SetWindowStyle( wxBORDER_NONE );
    content->SetEditable( false );
    sizer->Add( content, 1, wxEXPAND );

    panel->SetSizer( sizer );
}

void AboutDialog::createLicensePage( wxNotebook* nb )
{
    wxPanel* panel = new wxPanel( nb );
    nb->AddPage( panel, _( "License" ) );

    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
    wxHtmlWindow* content = new wxHtmlWindow( panel );
    content->SetWindowStyle( wxBORDER_NONE );
    content->Bind( wxEVT_HTML_LINK_CLICKED, &AboutDialog::onLinkClicked, this );
    sizer->Add( content, 1, wxEXPAND );

    wxFileName licenseName( wxStandardPaths::Get().GetExecutablePath() );
    licenseName.SetFullName( "LICENSE.html" );

    {
        wxLogNull ln;
        if ( !content->LoadFile( licenseName.GetFullPath() ) )
        {
            content->SetPage( _( "License file could not be loaded." ) );
        }
    }

    panel->SetSizer( sizer );
}

void AboutDialog::onDpiChanged( wxDPIChangedEvent& event )
{
    loadBitmap();
}

void AboutDialog::onPaintBitmap( wxPaintEvent& event )
{
    wxPaintDC dc( m_banner );
    dc.DrawBitmap( m_bannerBmp, wxPoint( 0, 0 ) );

    dc.SetTextForeground( *wxBLACK );
    dc.SetFont( m_versionFont );

    // Draw copyright information
    static const wxString COPYRIGHT_TEXT = L"(C) 2021-2022 £ukasz Œwiszcz";
    dc.DrawText( COPYRIGHT_TEXT, FromDIP( wxPoint( 185, 125 ) ) );

    // Draw version information
    // TRANSLATORS: version string (e.g. "ver. 1.2.3")
    const wxString VERSION = wxString::Format( _( "ver. %s" ), Utils::VERSION );
    int xOff = dc.GetTextExtent( VERSION ).x;
    dc.DrawText( VERSION, wxPoint( FromDIP( 465 ) - xOff, FromDIP( 125 ) ) );
}

void AboutDialog::onLinkClicked( wxHtmlLinkEvent& event )
{
    wxLaunchDefaultBrowser( event.GetLinkInfo().GetHref() );
}

};
