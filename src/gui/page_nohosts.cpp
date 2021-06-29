#include "page_nohosts.hpp"

#include "../../win32/resource.h"
#include "utils.hpp"


namespace gui
{

const wxString NoHostsPage::s_text = _( "There are no other computers"
    " found on your network..." );

NoHostsPage::NoHostsPage( wxWindow* parent )
    : wxPanel( parent, wxID_ANY )
    , m_icon( nullptr )
    , m_label( nullptr )
    , m_retryBtn( nullptr )
    , m_iconBmp( wxNullBitmap )
{
    wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );

    mainSizer->AddStretchSpacer();

    m_icon = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap );
    loadIcon();
    mainSizer->Add( m_icon, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 8 );

    m_label = new wxStaticText( this, wxID_ANY, NoHostsPage::s_text );
    wxFont labelFnt = m_label->GetFont();
    labelFnt.MakeLarger();
    labelFnt.MakeLarger();
    m_label->SetFont( labelFnt );
    mainSizer->Add( m_label, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 8 );

    m_retryBtn = new wxButton( this, wxID_ANY, _( "Try again" ) );
    int width = m_retryBtn->GetSize().x;
    m_retryBtn->SetMinSize( wxSize( width * 1.5f, FromDIP( 30 ) ) );
    m_retryBtn->SetWindowVariant( wxWINDOW_VARIANT_LARGE );
    mainSizer->Add( m_retryBtn, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 12 );

    mainSizer->AddStretchSpacer();

    SetSizer( mainSizer );

    // Events

    Bind( wxEVT_DPI_CHANGED, &NoHostsPage::onDpiChanged, this );
    m_retryBtn->Bind( wxEVT_BUTTON, &NoHostsPage::onRetryClicked, this );
}

void NoHostsPage::onDpiChanged( wxDPIChangedEvent& event )
{
    loadIcon();
}

void NoHostsPage::onRetryClicked( wxCommandEvent& event )
{
    wxMessageBox( wxT( "xd it works" ) );
}

void NoHostsPage::loadIcon()
{
    wxBitmap original;
    original.LoadFile( Utils::makeIntResource( IDB_INFO ),
        wxBITMAP_TYPE_PNG_RESOURCE );

    wxImage iconImg = original.ConvertToImage();

    int targetSize = FromDIP( 80 );

    m_iconBmp = iconImg.Scale( targetSize, targetSize,
        wxIMAGE_QUALITY_BICUBIC );
    m_icon->SetBitmap( m_iconBmp );
}

};
