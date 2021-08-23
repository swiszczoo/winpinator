#include "page_offline.hpp"

#include "../../win32/resource.h"
#include "utils.hpp"


namespace gui
{

wxString OfflinePage::TEXT = _( "It seems you're offline! Check your"
    " LAN connection..." );

OfflinePage::OfflinePage( wxWindow* parent )
    : wxPanel( parent, wxID_ANY )
    , m_icon( nullptr )
    , m_label( nullptr )
    , m_iconBmp( wxNullBitmap )
{
    wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );

    mainSizer->AddStretchSpacer();

    m_icon = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap );
    loadIcon();
    mainSizer->Add( m_icon, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 8 );

    m_label = new wxStaticText( this, wxID_ANY, OfflinePage::TEXT, 
        wxDefaultPosition, wxDefaultSize, wxELLIPSIZE_END );
    wxFont labelFont = m_label->GetFont();
    labelFont.MakeLarger();
    labelFont.MakeLarger();
    m_label->SetFont( labelFont );
    mainSizer->Add( m_label, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 8 );

    mainSizer->AddStretchSpacer();

    SetSizer( mainSizer );

    // Events

    Bind( wxEVT_DPI_CHANGED, &OfflinePage::onDpiChanged, this );
}

void OfflinePage::onDpiChanged( wxDPIChangedEvent& event )
{
    loadIcon();
}

void OfflinePage::loadIcon()
{
    wxBitmap original;
    original.LoadFile( Utils::makeIntResource( IDB_WARNING ),
        wxBITMAP_TYPE_PNG_RESOURCE );

    wxImage iconImg = original.ConvertToImage();

    int targetSize = FromDIP( 80 );

    m_iconBmp = iconImg.Scale( targetSize, targetSize, 
        wxIMAGE_QUALITY_BICUBIC );
    m_icon->SetBitmap( m_iconBmp );
}

};
