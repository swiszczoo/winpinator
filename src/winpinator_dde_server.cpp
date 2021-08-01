#include "winpinator_dde_server.hpp"
#include "winpinator_dde_consts.hpp"

#include <wx/log.h>

wxDEFINE_EVENT( EVT_OPEN_APP_WINDOW, wxCommandEvent );

DDEImpl::DDEImpl( DDEHandler* handler )
    : wxDDEServer()
    , m_handler( handler )
{
}

wxConnectionBase* DDEImpl::OnAcceptConnection( const wxString& topic )
{
    wxLogDebug( "Received DDE connection request." );

    if ( topic == DDEConsts::TOPIC_NAME )
    {
        DDEConnectionImpl* conn = new DDEConnectionImpl( m_handler );
        return conn;
    }

    wxLogDebug( "Unknown topic, refusing connection!" );
    return NULL;
}

DDEConnectionImpl::DDEConnectionImpl( DDEHandler* handler )
    : wxDDEConnection()
    , m_handler( handler )
{
}

bool DDEConnectionImpl::OnExecute( const wxString& topic,
    const void* data, size_t size, wxIPCFormat format )
{
    if ( format != wxIPCFormat::wxIPC_TEXT )
    {
        wxLogDebug( "Wrong data type, returning false..." );
        return false;
    }

    if ( topic != DDEConsts::TOPIC_NAME )
    {
        wxLogDebug( "Wrong topic, returning false..." );
        return false;
    }

    if ( m_handler )
    {
        m_handler->processExecute( data, size );
        return true;
    }

    return false;
}

WinpinatorDDEServer::WinpinatorDDEServer( wxEvtHandler* evtHandler )
    : m_handler( evtHandler )
    , m_server( nullptr )
{
    // We can't use handy std::make_unique here:
    m_server = std::unique_ptr<DDEImpl>( new DDEImpl( this ) );
    m_server->Create( DDEConsts::SERVICE_NAME );
}

void WinpinatorDDEServer::processExecute( const void* data, size_t size )
{
    wxString dataStr( (char*)data, size );

    if ( dataStr == DDEConsts::COMMAND_OPEN )
    {
        // Open main app window
        wxCommandEvent openEv( EVT_OPEN_APP_WINDOW );
        wxPostEvent( m_handler, openEv );
    }
}

