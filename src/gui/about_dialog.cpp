#include "about_dialog.hpp"

#include "../../win32/resource.h"
#include "utils.hpp"

#include <wx/filename.h>
#include <wx/mstream.h>
#include <wx/notebook.h>
#include <wx/richtext/richtextxml.h>
#include <wx/stdpaths.h>
#include <wx/xml/xml.h>

namespace gui
{

const wxString AboutDialog::ABOUT_XML = wxTRANSLATE(
    // TRANSLATORS: This XML is loaded into wxRichTextCtrl in "About" section
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<richtext version=\"1.0.0.0\" xmlns=\"http://www.wxwidgets.org\">\n"
    "  <paragraphlayout textcolor=\"#000000\" fontpointsize=\"12\" fontfamily=\"72\" "
    "fontstyle=\"90\" fontweight=\"400\" fontunderlined=\"0\" "
    "fontface=\"Times New Roman\" alignment=\"1\" parspacingafter=\"10\" "
    "parspacingbefore=\"0\" linespacing=\"10\" margin-left=\"10,4098\" "
    "margin-right=\"10,4098\" margin-top=\"10,4098\" margin-bottom=\"10,4098\">\n"
    "    <paragraph fontpointsize=\"10\" fontweight=\"700\" fontface=\"Segoe UI\">\n"
    "      <text fontpointsize=\"15\" fontstyle=\"93\" fontweight=\"700\" "
    "fontface=\"Segoe UI\">About</text>\n"
    "    </paragraph>\n"
    "    <paragraph fontpointsize=\"10\" fontweight=\"700\" fontface=\"Segoe UI\">\n"
    "      <text fontpointsize=\"10\" fontweight=\"700\" "
    "fontface=\"Segoe UI\"></text>\n"
    "    </paragraph>\n"
    "    <paragraph>\n"
    "      <text fontpointsize=\"10\" fontweight=\"700\" fontface=\"Segoe UI\">"
    "\"Winpinator \"</text>\n"
    "      <text fontpointsize=\"10\" fontweight=\"400\" fontface=\"Segoe UI\">"
    "is an unofficial Windows port of a Linux file transfer tool called Warpinator. "
    "It provides all features of its Linux equivalent, however, Windows uses its "
    "own filesystem called NTFS and is generally architecturally different, hence "
    "Winpinator has to emulate some Linux features, like permission system. "
    "Winpinator should work out-of-the-box, it makes use of zeroconf networking, "
    "i.e. mDNS protocol, but some firewall settings might prevent it from "
    "communicating with the rest of the network.</text>\n"
    "    </paragraph>\n"
    "    <paragraph fontweight=\"400\">\n"
    "      <text fontpointsize=\"10\" fontface=\"Segoe UI\"></text>\n"
    "    </paragraph>\n"
    "    <paragraph fontweight=\"400\">\n"
    "      <text fontpointsize=\"10\" fontface=\"Segoe UI\">The main goal of the "
    "project was to create a tool that provides native Windows look-and-feel, so "
    "the interface uses default system skin and supports drag-and-drop.</text>\n"
    "    </paragraph>\n"
    "    <paragraph fontweight=\"400\">\n"
    "      <text fontpointsize=\"10\" fontface=\"Segoe UI\"></text>\n"
    "    </paragraph>\n"
    "    <paragraph fontweight=\"400\">\n"
    "      <text fontpointsize=\"15\" fontstyle=\"93\" fontweight=\"700\" "
    "fontface=\"Segoe UI\">Used libraries</text>\n"
    "    </paragraph>\n"
    "    <paragraph fontweight=\"400\">\n"
    "      <text fontpointsize=\"15\" fontstyle=\"93\" fontweight=\"700\" "
    "fontface=\"Segoe UI\"></text>\n"
    "    </paragraph>\n"
    "    <paragraph fontweight=\"400\" leftindent=\"60\" leftsubindent=\"60\" "
    "bulletstyle=\"512\" bulletname=\"standard/circle\" liststyle=\"Bullet List 1\">\n"
    "      <text fontpointsize=\"10\" fontweight=\"400\" fontface=\"Segoe UI\">"
    "wxWidgets - as a general framework, to provide native Windows look-and-feel</text>\n"
    "    </paragraph>\n"
    "    <paragraph fontweight=\"400\" leftindent=\"60\" leftsubindent=\"60\" "
    "bulletstyle=\"512\" bulletname=\"standard/circle\" liststyle=\"Bullet List 1\">\n"
    "      <text fontpointsize=\"10\" fontface=\"Segoe UI\">gRPC and protobuf - "
    "to support the protocol of official Warpinator</text>\n"
    "    </paragraph>\n"
    "    <paragraph fontweight=\"400\" leftindent=\"60\" leftsubindent=\"60\" "
    "bulletstyle=\"512\" bulletname=\"standard/circle\" liststyle=\"Bullet List 1\">\n"
    "      <text fontpointsize=\"10\" fontface=\"Segoe UI\">sodium - to encrypt "
    "and decrypt RSA keys with the symmetric group key</text>\n"
    "    </paragraph>\n"
    "    <paragraph fontweight=\"400\" leftindent=\"60\" leftsubindent=\"60\" "
    "bulletstyle=\"512\" bulletname=\"standard/circle\" liststyle=\"Bullet List 1\">\n"
    "      <text fontpointsize=\"10\" fontface=\"Segoe UI\">openssl - to generate "
    "safe PEM keys</text>\n"
    "    </paragraph>\n"
    "    <paragraph fontweight=\"400\" leftindent=\"60\" leftsubindent=\"60\" "
    "bulletstyle=\"512\" bulletname=\"standard/circle\" liststyle=\"Bullet List 1\">\n"
    "      <text fontpointsize=\"10\" fontface=\"Segoe UI\">sqlite3 - to store "
    "persistent data, e.g. transfer history</text>\n"
    "    </paragraph>\n"
    "    <paragraph fontweight=\"400\" leftindent=\"60\" leftsubindent=\"60\" "
    "bulletstyle=\"512\" bulletname=\"standard/circle\" liststyle=\"Bullet List 1\">\n"
    "      <text fontpointsize=\"10\" fontface=\"Segoe UI\">zlib - to support "
    "transfer compression (one of the newer features of Warpinator)</text>\n"
    "    </paragraph>\n"
    "    <paragraph fontweight=\"400\" leftindent=\"60\" leftsubindent=\"60\" "
    "bulletstyle=\"512\" bulletname=\"standard/circle\" liststyle=\"Bullet List 1\">\n"
    "      <text fontpointsize=\"10\" fontface=\"Segoe UI\">wintoast - to display "
    "toast notifications</text>\n"
    "    </paragraph>\n"
    "    <paragraph fontweight=\"400\" leftindent=\"60\" leftsubindent=\"60\" "
    "bulletstyle=\"512\" bulletname=\"standard/circle\" liststyle=\"Bullet List 1\">\n"
    "      <text fontpointsize=\"10\" fontface=\"Segoe UI\">cpp-base64 - to "
    "convert binary keys to text and vice versa</text>\n"
    "    </paragraph>\n"
    "  </paragraphlayout>\n"
    "</richtext>\n" );

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

void AboutDialog::loadAboutText( wxRichTextCtrl* rt )
{
    wxLogNull logNull;

    bool ok = false;
    wxXmlDocument doc;
    wxString translated = wxGetTranslation( ABOUT_XML );
    std::string utf8 = translated.utf8_string();
    wxMemoryInputStream mis( translated.data(), translated.size() );
    wxRichTextXMLHandler handler;
    wxRichTextBuffer* buffer = &rt->GetBuffer();

    buffer->ResetAndClearCommands();
    buffer->Clear();

    if ( doc.Load( mis ) )
    {
        if ( doc.GetRoot() && doc.GetRoot()->GetType() == wxXML_ELEMENT_NODE 
            && doc.GetRoot()->GetName() == wxT( "richtext" ) )
        {
            wxXmlNode* child = doc.GetRoot()->GetChildren();
            while ( child )
            {
                if ( child->GetType() == wxXML_ELEMENT_NODE )
                {
                    wxString name = child->GetName();
                    if ( name == wxT( "richtext-version" ) )
                    {
                    }
                    else
                        handler.ImportXML( buffer, buffer, child );
                }

                child = child->GetNext();
            }

            ok = true;
        }
    }
    
    if ( !ok )
    {
        rt->SetValue( _( "Could not load About section." ) );
    }
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

    loadAboutText( content );

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
