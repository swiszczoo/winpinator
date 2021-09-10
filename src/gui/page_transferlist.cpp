#include "page_transferlist.hpp"

#include "../../win32/resource.h"
#include "../globals.hpp"
#include "utils.hpp"

#include <wx/translation.h>

namespace gui
{

wxDEFINE_EVENT( EVT_GO_BACK, wxCommandEvent );

TransferListPage::TransferListPage( wxWindow* parent, const wxString& targetId )
    : wxPanel( parent, wxID_ANY ) 
    , m_target( targetId )
    , m_header( nullptr )
    , m_details( nullptr )
    , m_backBtn( nullptr )
    , m_fileBtn( nullptr )
    , m_directoryBtn( nullptr )
    , m_opPanel( nullptr )
    , m_opList( nullptr )
    , m_statusLabel( nullptr )
{
    auto srv = Globals::get()->getWinpinatorServiceInstance();

    wxBoxSizer* margSizer = new wxBoxSizer( wxHORIZONTAL );
    
    margSizer->AddSpacer( FromDIP( 20 ) );
    margSizer->AddStretchSpacer();

    wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );

    m_header = new wxStaticText( this, wxID_ANY,
        _( "Select files or directories to start transfer" ) );
    m_header->SetFont( Utils::get()->getHeaderFont() );
    m_header->SetForegroundColour( Utils::get()->getHeaderColor() );
    mainSizer->Add( m_header, 0, wxEXPAND | wxTOP, FromDIP( 25 ) );

    m_details = new wxStaticText( this, wxID_ANY,
        _( "Drop elements you want to send into the box below "
           "or click one of the buttons." ) );
    mainSizer->Add( m_details, 0, wxEXPAND | wxTOP | wxBOTTOM, FromDIP( 4 ) );

    wxBoxSizer* buttonSizer = new wxBoxSizer( wxHORIZONTAL );

    m_backBtn = new ToolButton( this, wxID_ANY, _( "Go back" ) );
    m_backBtn->SetWindowStyle( wxBU_EXACTFIT );
    buttonSizer->Add( m_backBtn, 0, wxEXPAND );

    buttonSizer->AddStretchSpacer();

    m_fileBtn = new ToolButton( this, wxID_ANY, _( "Send file(s)..." ) );
    m_fileBtn->SetWindowStyle( wxBU_EXACTFIT );
    buttonSizer->Add( m_fileBtn, 0, wxEXPAND | wxRIGHT, FromDIP( 4 ) );

    m_directoryBtn = new ToolButton( this, wxID_ANY, _( "Send a folder..." ) );
    m_directoryBtn->SetWindowStyle( wxBU_EXACTFIT );
    buttonSizer->Add( m_directoryBtn, 0, wxEXPAND );

    mainSizer->Add( buttonSizer, 0, wxTOP | wxEXPAND, FromDIP( 8 ) );

    mainSizer->AddSpacer( FromDIP( 4 ) );

    m_opPanel = new wxPanel( this );

    wxBoxSizer* opSizer = new wxBoxSizer( wxVERTICAL );
    m_opPanel->SetSizer( opSizer );

    m_opList = new ScrolledTransferHistory( m_opPanel );
    m_opList->SetBackgroundColour( 
        wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

    opSizer->Add( m_opList, 1, wxEXPAND | wxTOP | wxLEFT | wxRIGHT, 1 );

    mainSizer->Add( m_opPanel, 1, wxEXPAND );

    m_statusLabel = new StatusText( this );
    srv::RemoteInfoPtr currentInfo = srv->getRemoteManager()
        ->getRemoteInfo( targetId );

    if ( currentInfo )
    {
        updateForStatus( currentInfo->state );
    }

    mainSizer->Add( m_statusLabel, 0, wxBOTTOM | wxEXPAND, FromDIP( 30 ) );

    margSizer->Add( mainSizer, 10, wxEXPAND );

    margSizer->AddStretchSpacer();
    margSizer->AddSpacer( FromDIP( 20 ) );

    loadIcons();
    m_backBtn->SetBitmapMargins( FromDIP( 1 ), FromDIP( 1 ) );
    m_fileBtn->SetBitmapMargins( FromDIP( 1 ), FromDIP( 1 ) );
    m_directoryBtn->SetBitmapMargins( FromDIP( 1 ), FromDIP( 1 ) );

    SetSizer( margSizer );

    observeService( srv );

    // Events 
    Bind( wxEVT_DPI_CHANGED, &TransferListPage::onDpiChanged, this );
    m_backBtn->Bind( wxEVT_BUTTON, &TransferListPage::onBackClicked, this );
    Bind( wxEVT_THREAD, &TransferListPage::onUpdateStatus, this );
}

void TransferListPage::loadIcons()
{
    loadSingleIcon( Utils::makeIntResource( IDB_BACK ), 
        &m_backBmp, m_backBtn );
    loadSingleIcon( Utils::makeIntResource( IDB_SEND_FILE ), 
        &m_fileBmp, m_fileBtn );
    loadSingleIcon( Utils::makeIntResource( IDB_SEND_DIR ), 
        &m_dirBmp, m_directoryBtn );
}

void TransferListPage::loadSingleIcon( const wxString& res, 
    wxBitmap* loc, ToolButton* btn )
{
    const int size = FromDIP( 24 ); 

    wxBitmap original;
    original.LoadFile( res, wxBITMAP_TYPE_PNG_RESOURCE );

    wxImage img = original.ConvertToImage();

    *loc = img.Scale( size, size, wxIMAGE_QUALITY_BICUBIC );
    btn->SetBitmap( *loc, wxDirection::wxWEST );
}

void TransferListPage::onDpiChanged( wxDPIChangedEvent& event )
{
    loadIcons();
}

void TransferListPage::onBackClicked( wxCommandEvent& event )
{
    // Repost this as a EVT_GO_BACK event

    wxCommandEvent evnt( EVT_GO_BACK );
    wxPostEvent( this, evnt );
}

void TransferListPage::onUpdateStatus( wxThreadEvent& event )
{
    srv::RemoteInfoPtr info = event.GetPayload<srv::RemoteInfoPtr>();
    updateForStatus( info->state );
}

void TransferListPage::onStateChanged()
{
    // Stub!
}

void TransferListPage::onEditHost( srv::RemoteInfoPtr newInfo )
{
    if ( newInfo->id == m_target )
    {
        wxThreadEvent evnt( wxEVT_THREAD );
        evnt.SetPayload( newInfo );
        wxQueueEvent( this, evnt.Clone() );
    }
}

void TransferListPage::updateForStatus( srv::RemoteStatus status )
{
    m_statusLabel->setStatus( status );
    m_opPanel->SetBackgroundColour( m_statusLabel->getBarColor() );
    m_opPanel->Refresh();

    bool enableSending = ( status == srv::RemoteStatus::ONLINE );

    m_fileBtn->Enable( enableSending );
    m_directoryBtn->Enable( enableSending );
}

};
