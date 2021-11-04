#include "page_error.hpp"

#include "../../win32/resource.h"
#include "../globals.hpp"
#include "../main_base.hpp"
#include "utils.hpp"

namespace gui
{

wxDEFINE_EVENT( EVT_OPEN_SETTINGS, wxCommandEvent );

const wxString ErrorPage::TEXT = wxTRANSLATE( "A fatal error occurred while trying "
                                              "to start the service!" );

ErrorPage::ErrorPage( wxWindow* parent )
    : wxPanel( parent, wxID_ANY )
    , m_srvError( srv::ServiceError::KEIN_ERROR )
    , m_icon( nullptr )
    , m_label( nullptr )
    , m_description( nullptr )
    , m_retryBtn( nullptr )
    , m_iconBmp( wxNullBitmap )
{
    wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );

    mainSizer->AddStretchSpacer();

    m_icon = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap );
    loadIcon();
    mainSizer->Add( m_icon, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 8 );

    m_label = new wxStaticText( this, wxID_ANY, wxGetTranslation( TEXT ) );
    wxFont labelFnt = m_label->GetFont();
    labelFnt.MakeLarger();
    labelFnt.MakeLarger();
    m_label->SetFont( labelFnt );
    mainSizer->Add( m_label, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 8 );

    mainSizer->AddSpacer( 8 );

    m_description = new wxTextCtrl( this, wxID_ANY, "XD", 
        wxDefaultPosition, wxDefaultSize, 
        wxBORDER_NONE | wxTE_MULTILINE | wxTE_READONLY 
        | wxTE_NO_VSCROLL | wxTE_CENTRE );
    m_description->SetMinSize( FromDIP( wxSize( 64, 64 ) ) );
    m_description->SetSize( FromDIP( wxSize( 64, 64 ) ) );
    updateLabel();
    mainSizer->Add( m_description, 0, wxEXPAND | wxLEFT | wxRIGHT, 96 );

    mainSizer->AddSpacer( 8 );

    m_retryBtn = new wxButton( this, wxID_ANY, _( "Try again" ) );
    int retryWidth = m_retryBtn->GetSize().x;
    m_retryBtn->SetMinSize( wxSize( retryWidth * 1.5f, FromDIP( 30 ) ) );
    m_retryBtn->SetWindowVariant( wxWINDOW_VARIANT_LARGE );
    mainSizer->Add( m_retryBtn, 0, wxALIGN_CENTER_HORIZONTAL | wxTOP, 12 );

    m_settingsBtn = new wxButton( this, wxID_ANY, _( "Open settings" ) );
    int settingsWidth = m_settingsBtn->GetSize().x;
    m_settingsBtn->SetMinSize( wxSize( settingsWidth * 1.5f, FromDIP( 30 ) ) );
    m_settingsBtn->SetWindowVariant( wxWINDOW_VARIANT_LARGE );
    mainSizer->Add( m_settingsBtn, 0, wxALIGN_CENTER_HORIZONTAL | wxTOP, 12 );

    mainSizer->AddStretchSpacer();

    SetSizer( mainSizer );

    // Events

    Bind( wxEVT_DPI_CHANGED, &ErrorPage::onDpiChanged, this );
    m_retryBtn->Bind( wxEVT_BUTTON, &ErrorPage::onRetryClicked, this );
    m_settingsBtn->Bind( wxEVT_BUTTON, &ErrorPage::onSettingsClicked, this );
}

void ErrorPage::setServiceError( srv::ServiceError err ) 
{
    m_srvError = err;
    updateLabel();
}

srv::ServiceError ErrorPage::getServiceError() const
{
    return m_srvError;
}

void ErrorPage::onDpiChanged( wxDPIChangedEvent& event )
{
    loadIcon();
}

void ErrorPage::onRetryClicked( wxCommandEvent& event )
{
    srv::Event evnt;
    evnt.type = srv::EventType::RESTART_SERVICE;
    evnt.eventData.restartData = std::make_shared<SettingsModel>( 
        GetApp().m_settings );

    Globals::get()->getWinpinatorServiceInstance()->postEvent( evnt );

    wxPostEvent( this, event );
}

void ErrorPage::onSettingsClicked( wxCommandEvent& event )
{
    wxCommandEvent evnt( EVT_OPEN_SETTINGS );
    wxPostEvent( this, evnt );
}

void ErrorPage::loadIcon()
{
    wxBitmap original;
    original.LoadFile( Utils::makeIntResource( IDB_ERROR ),
        wxBITMAP_TYPE_PNG_RESOURCE );

    wxImage iconImg = original.ConvertToImage();

    int targetSize = FromDIP( 80 );

    m_iconBmp = iconImg.Scale( targetSize, targetSize,
        wxIMAGE_QUALITY_BICUBIC );
    m_icon->SetBitmap( m_iconBmp );
}

void ErrorPage::updateLabel()
{
    switch ( m_srvError )
    {
    case srv::ServiceError::KEIN_ERROR:
        m_description->SetLabel( _( "No error." ) );
        break;
    case srv::ServiceError::UNKNOWN_ERROR:
        m_description->SetLabel( _( "Unknown error!" ) );
        break;
    case srv::ServiceError::ZEROCONF_SERVER_FAILED:
        m_description->SetLabel( _(
            "Can't create mDNS server! Maybe another "
            "running app is holding exclusive access to appropriate port?" ) );
        break;
    case srv::ServiceError::REGISTRATION_V1_SERVER_FAILED:
        m_description->SetLabel( _(
            "Can't create registration service (for compatibility mode). "
            "Maybe there's another instance of Winpinator running on "
            "your computer, e.g. started by another user? "
            "You can always try to change ports in the settings." ) );
        break;
    case srv::ServiceError::REGISTRATION_V2_SERVER_FAILED:
        m_description->SetLabel( _(
            "Can't create registration service. "
            "Maybe there's another instance of Winpinator running on "
            "your computer, e.g. started by another user? "
            "You can always try to change ports in the settings." ) );
        break;
    case srv::ServiceError::WARP_SERVER_FAILED:
        m_description->SetLabel( _(
            "Can't create main service. "
            "Maybe there's another instance of Winpinator running on "
            "your computer, e.g. started by another user? "
            "You can always try to change ports in the settings." ) );
        break;
    case srv::ServiceError::CANT_CREATE_OUTPUT_DIR:
        m_description->SetLabel( _(
            "Output directory you provided in the settings does not exist. "
            "Winpinator failed to create one."
            ) );
        break;
    default:
        assert( false );
        m_description->SetLabel( _( "Missing description" ) );
        break;
    }
}

};
