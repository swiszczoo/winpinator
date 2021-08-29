#include "page_hostlist.hpp"

#include "../../win32/resource.h"
#include "../globals.hpp"
#include "utils.hpp"

#include <wx/mstream.h>
#include <wx/tooltip.h>
#include <wx/translation.h>

#include <mutex>

namespace gui
{

wxDEFINE_EVENT( EVT_NO_HOSTS_IN_TIME, wxCommandEvent );
wxDEFINE_EVENT( EVT_TARGET_SELECTED, wxCommandEvent );

const wxString HostListPage::DETAILS = _(
    "Below is a list of currently available computers. "
    "Select the one you want to transfer your files to." );

const wxString HostListPage::DETAILS_WRAPPED = _(
    "Below is a list of currently available computers.\n"
    "Select the one you want to transfer your files to." );

const int HostListPage::NO_HOSTS_TIMEOUT_MILLIS = 15000;

HostListPage::HostListPage( wxWindow* parent )
    : wxPanel( parent, wxID_ANY )
    , m_header( nullptr )
    , m_details( nullptr )
    , m_refreshBtn( nullptr )
    , m_hostlist( nullptr )
    , m_fwdBtn( nullptr )
    , m_progLbl( nullptr )
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

    m_details = new wxStaticText( this, wxID_ANY, HostListPage::DETAILS,
        wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_END );
    headingSizerV->Add( m_details, 0, wxEXPAND | wxTOP | wxBOTTOM, FromDIP( 4 ) );

    headingSizerH->Add( headingSizerV, 1, wxEXPAND | wxRIGHT, FromDIP( 15 ) );

    m_refreshBtn = new ToolButton( this, wxID_ANY, wxEmptyString );
    m_refreshBtn->SetToolTip( new wxToolTip( _( "Refresh list" ) ) );
    m_refreshBtn->SetWindowVariant( wxWindowVariant::wxWINDOW_VARIANT_LARGE );
    loadIcon();
    m_refreshBtn->SetWindowStyle( wxBU_EXACTFIT );
    m_refreshBtn->SetBitmapMargins( FromDIP( 1 ), FromDIP( 1 ) );
    headingSizerH->Add( m_refreshBtn, 0, wxALIGN_BOTTOM );

    mainSizer->Add( headingSizerH, 0, wxEXPAND );

    m_hostlist = new HostListbox( this );
    m_hostlist->SetWindowStyle( wxBORDER_THEME | wxLB_SINGLE );
    mainSizer->Add( m_hostlist, 1, wxEXPAND | wxTOP | wxBOTTOM, FromDIP( 10 ) );

    wxBoxSizer* bottomBar = new wxBoxSizer( wxHORIZONTAL );

    m_progLbl = new ProgressLabel( this, 
        _( "Searching for computers on your network..." ) );
    bottomBar->Add( m_progLbl, 0, wxALIGN_CENTER_VERTICAL, 0 );

    bottomBar->AddStretchSpacer();

    m_fwdBtn = new wxButton( this, wxID_ANY, _( "&Next >" ) );
    m_fwdBtn->SetMinSize( wxSize( m_fwdBtn->GetSize().x * 1.5, FromDIP( 25 ) ) );
    m_fwdBtn->Disable();
    bottomBar->Add( m_fwdBtn, 0, wxALIGN_CENTER_VERTICAL, 0 );

    mainSizer->Add( bottomBar, 0, wxEXPAND | wxBOTTOM, FromDIP( 25 ) );

    margSizer->Add( mainSizer, 10, wxEXPAND );

    margSizer->AddStretchSpacer();
    margSizer->AddSpacer( FromDIP( 20 ) );

    SetSizer( margSizer );

    m_timer.SetOwner( this );
    refreshAll();

    observeService( Globals::get()->getWinpinatorServiceInstance() );

    // Events

    Bind( wxEVT_DPI_CHANGED, &HostListPage::onDpiChanged, this );
    Bind( wxEVT_SIZE, &HostListPage::onLabelResized, this );
    Bind( wxEVT_BUTTON, &HostListPage::onRefreshClicked, this );
    Bind( wxEVT_TIMER, &HostListPage::onTimerTicked, this );
    Bind( wxEVT_THREAD, &HostListPage::onManipulateList, this );
    m_hostlist->Bind( wxEVT_LISTBOX, 
        &HostListPage::onHostSelectionChanged, this );
    m_fwdBtn->Bind( wxEVT_BUTTON, &HostListPage::onNextClicked, this );
}

void HostListPage::refreshAll()
{
    m_timer.StartOnce( HostListPage::NO_HOSTS_TIMEOUT_MILLIS );

    // Remember currently selected item id
    const wxString& currentId = m_hostlist->getSelectedIdent();

    // Get current service state (host list)
    auto serv = Globals::get()->getWinpinatorServiceInstance();

    m_trackedRemotes = serv->getRemoteManager()->generateCurrentHostList();
    
    m_hostlist->clear();

    size_t count = 0;
    int selmark = wxNOT_FOUND;
    for ( srv::RemoteInfoPtr remote : m_trackedRemotes )
    {
        m_hostlist->addItem( convertRemoteInfoToHostItem( remote ) );

        if ( remote->id == currentId )
        {
            selmark = (int)count;
        }

        count++;
    }

    m_hostlist->SetSelection( selmark );
    m_fwdBtn->Enable( selmark != wxNOT_FOUND );
}

void HostListPage::onDpiChanged( wxDPIChangedEvent& event )
{
    loadIcon();
}

void HostListPage::onLabelResized( wxSizeEvent& event )
{
    int textWidth = m_details->GetTextExtent( HostListPage::DETAILS ).x;

    if ( textWidth > m_details->GetSize().x )
    {
        m_details->SetLabel( HostListPage::DETAILS_WRAPPED );
    }
    else
    {
        m_details->SetLabel( HostListPage::DETAILS );
    }

    event.Skip();
}

void HostListPage::onRefreshClicked( wxCommandEvent& event )
{
    srv::Event srvEvent;
    srvEvent.type = srv::EventType::REPEAT_MDNS_QUERY;

    Globals::get()->getWinpinatorServiceInstance()->postEvent( srvEvent );
}

void HostListPage::onTimerTicked( wxTimerEvent& event )
{
    auto serv = Globals::get()->getWinpinatorServiceInstance();

    if ( serv->getRemoteManager()->getVisibleHostsCount() == 0 )
    {
        wxCommandEvent evt( EVT_NO_HOSTS_IN_TIME );
        wxPostEvent( this, evt );
    }
}

void HostListPage::onManipulateList( wxThreadEvent& event )
{
    switch ( (ThreadEventType)event.GetInt() )
    {
    case ThreadEventType::ADD:
    {
        srv::RemoteInfoPtr info = event.GetPayload<srv::RemoteInfoPtr>();
        std::lock_guard<std::mutex> guard( info->mutex );

        for ( const srv::RemoteInfoPtr& ptr : m_trackedRemotes )
        {
            if ( info->id == ptr->id )
            {
                m_hostlist->updateItemById( 
                    convertRemoteInfoToHostItem( info ) );
                return;
            }
        }

        m_trackedRemotes.push_back( info );
        m_hostlist->addItem( convertRemoteInfoToHostItem( info ) );

        break;
    }
    case ThreadEventType::UPDATE:
    {
        srv::RemoteInfoPtr info = event.GetPayload<srv::RemoteInfoPtr>();
        std::lock_guard<std::mutex> guard( info->mutex );

        m_hostlist->updateItemById( convertRemoteInfoToHostItem( info ) );
        break;
    }
    case ThreadEventType::RESET:
    {
        m_trackedRemotes.clear();
        m_hostlist->clear();

        break;
    }
    
    }
    
}

void HostListPage::onHostSelectionChanged( wxCommandEvent& event )
{
    const wxString& ident = m_hostlist->getSelectedIdent();

    if ( !ident.empty() )
    {
        m_fwdBtn->Enable();
    }
    else
    {
        m_fwdBtn->Disable();
    }
}

void HostListPage::onNextClicked( wxCommandEvent& event )
{
    const wxString& selId = m_hostlist->getSelectedIdent();

    if ( !selId.empty() )
    {
        // Post an event to notify about target change

        wxCommandEvent evnt( EVT_TARGET_SELECTED );
        evnt.SetString( selId );
        wxPostEvent( this, evnt );
    }
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

HostItem HostListPage::convertRemoteInfoToHostItem( srv::RemoteInfoPtr rinfo )
{
    HostItem result;
    result.id = rinfo->id;

    if ( rinfo->shortName.empty() )
    {
        result.hostname = rinfo->hostname;
    }
    else
    {
        result.hostname = rinfo->shortName + '@' + rinfo->hostname;
    }

    result.ipAddress = rinfo->ips.ipv4;

    if ( result.ipAddress.empty() )
    {
        result.ipAddress = rinfo->ips.ipv6;
    }

    result.os = rinfo->os;
    // We set the bitmap to null to invalidate it
    result.profileBmp = std::make_shared<wxBitmap>( wxNullBitmap );

    if ( rinfo->avatarBuffer.empty() )
    {
        result.profilePic = std::make_shared<wxImage>( wxNullImage );
        m_cachedAvatars.erase( rinfo->id );
        m_avatarCache.erase( rinfo->id );
    }
    else if ( rinfo->avatarBuffer == m_cachedAvatars[rinfo->id] )
    {
        result.profilePic = m_avatarCache[rinfo->id];
    }
    else
    {
        wxLogNull logNull;

        wxImage loader;
        wxMemoryInputStream inStream( rinfo->avatarBuffer.data(), 
            rinfo->avatarBuffer.size() );

        loader.LoadFile( inStream, wxBITMAP_TYPE_ANY );

        if ( loader.IsOk() )
        {
            result.profilePic = std::make_shared<wxImage>( loader );
            m_cachedAvatars[rinfo->id] = rinfo->avatarBuffer;
            m_avatarCache[rinfo->id] = result.profilePic;
        }
    }
    
    result.state = rinfo->state;

    if ( rinfo->state == srv::RemoteStatus::ONLINE )
    {
        result.username = rinfo->fullName;
    }
    else if ( rinfo->state == srv::RemoteStatus::UNREACHABLE
        || rinfo->state == srv::RemoteStatus::OFFLINE )
    {
        if ( rinfo->fullName.empty() )
        {
            result.username = _( "Data unavailable" );
        }
        else
        {
            result.username = rinfo->fullName;
        }
    }
    else
    {
        result.username = _( "Loading..." );
    }

    return result;
}

void HostListPage::onStateChanged()
{
    auto serv = Globals::get()->getWinpinatorServiceInstance();

    if ( !serv->isOnline() )
    {
        wxThreadEvent evnt;
        evnt.SetInt( (int)ThreadEventType::RESET );

        wxQueueEvent( this, evnt.Clone() );
    }
}

void HostListPage::onAddHost( srv::RemoteInfoPtr info )
{
    wxThreadEvent evnt;
    evnt.SetInt( (int)ThreadEventType::ADD );
    evnt.SetPayload( info );

    wxQueueEvent( this, evnt.Clone() );
}

void HostListPage::onEditHost( srv::RemoteInfoPtr info )
{
    wxThreadEvent evnt;
    evnt.SetInt( (int)ThreadEventType::UPDATE );
    evnt.SetPayload( info );

    wxQueueEvent( this, evnt.Clone() );
}

};
