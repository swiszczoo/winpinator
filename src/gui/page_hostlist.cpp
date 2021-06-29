#include "page_hostlist.hpp"

#include "../../win32/resource.h"
#include "utils.hpp"

#include <wx/tooltip.h>

namespace gui
{

const wxString HostListPage::s_details = _(
    "Below is a list of currently available computers. "
    "Select the one you want to transfer your files to." );

const wxString HostListPage::s_detailsWrapped = _(
    "Below is a list of currently available computers.\n"
    "Select the one you want to transfer your files to." );

HostListPage::HostListPage( wxWindow* parent )
    : wxPanel( parent, wxID_ANY )
    , m_header( nullptr )
    , m_details( nullptr )
    , m_refreshBtn( nullptr )
    , m_hostlist( nullptr )
    , m_fwdBtn( nullptr )
    , m_refreshBmp( wxNullBitmap )
{
    wxBoxSizer* margSizer = new wxBoxSizer( wxHORIZONTAL );

    margSizer->AddSpacer( FromDIP( 20 ) );
    margSizer->AddStretchSpacer();

    wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer* headingSizerH = new wxBoxSizer( wxHORIZONTAL );
    wxBoxSizer* headingSizerV = new wxBoxSizer( wxVERTICAL );

    m_header = new wxStaticText( this, wxID_ANY,
        _( "Select where to send your files" ) );
    m_header->SetFont( Utils::get()->getHeaderFont() );
    m_header->SetForegroundColour( Utils::get()->getHeaderColor() );
    headingSizerV->Add( m_header, 0, wxEXPAND | wxTOP, FromDIP( 25 ) );

    m_details = new wxStaticText( this, wxID_ANY, HostListPage::s_details,
        wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_END );
    headingSizerV->Add( m_details, 0, wxEXPAND | wxTOP | wxBOTTOM, FromDIP( 4 ) );

    headingSizerH->Add( headingSizerV, 1, wxEXPAND | wxRIGHT, FromDIP( 15 ) );

    m_refreshBtn = new ToolButton( this, wxID_ANY );
    m_refreshBtn->SetToolTip( new wxToolTip( _( "Refresh list" ) ) );
    m_refreshBtn->SetWindowVariant( wxWindowVariant::wxWINDOW_VARIANT_LARGE );
    loadIcon();
    m_refreshBtn->SetWindowStyle( wxBU_EXACTFIT );
    m_refreshBtn->SetBitmapMargins( FromDIP( 2 ), FromDIP( 2 ) );
    headingSizerH->Add( m_refreshBtn, 0, wxALIGN_BOTTOM );

    mainSizer->Add( headingSizerH, 0, wxEXPAND );

    m_hostlist = new HostListbox( this );
    m_hostlist->SetWindowStyle( wxBORDER_THEME );
    mainSizer->Add( m_hostlist, 1, wxEXPAND | wxTOP | wxBOTTOM, FromDIP( 10 ) );

    m_fwdBtn = new wxButton( this, wxID_ANY, _( "&Next >" ) );
    m_fwdBtn->SetMinSize( wxSize( m_fwdBtn->GetSize().x * 1.5, FromDIP( 25 ) ) );
    //m_fwdBtn->Disable();
    mainSizer->Add( m_fwdBtn, 0, wxALIGN_RIGHT | wxBOTTOM, FromDIP( 25 ) );

    margSizer->Add( mainSizer, 10, wxEXPAND );

    margSizer->AddStretchSpacer();
    margSizer->AddSpacer( FromDIP( 20 ) );

    SetSizer( margSizer );

    // Events

    Bind( wxEVT_DPI_CHANGED, &HostListPage::onDpiChanged, this );
    Bind( wxEVT_SIZE, &HostListPage::onLabelResized, this );
}

void HostListPage::onDpiChanged( wxDPIChangedEvent& event )
{
    loadIcon();
}

void HostListPage::onLabelResized( wxSizeEvent& event )
{
    int textWidth = m_details->GetTextExtent( HostListPage::s_details ).x;

    if ( textWidth > m_details->GetSize().x )
    {
        m_details->SetLabel( HostListPage::s_detailsWrapped );
    }
    else
    {
        m_details->SetLabel( HostListPage::s_details );
    }

    event.Skip();
}

void HostListPage::loadIcon()
{
    wxBitmap original;
    original.LoadFile( Utils::makeIntResource( IDB_REFRESH ),
        wxBITMAP_TYPE_PNG_RESOURCE );

    wxImage toScale = original.ConvertToImage();

    int size = FromDIP( 24 );

    m_refreshBmp = toScale.Scale( size, size, wxIMAGE_QUALITY_BICUBIC );
    m_refreshBtn->SetBitmap( m_refreshBmp, wxDirection::wxWEST );
}

};
