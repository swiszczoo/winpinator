#include "winpinator_dde_server.hpp"
#include "winpinator_dde_consts.hpp"

#include <wx/log.h>

#include <vector>

wxDEFINE_EVENT( EVT_OPEN_APP_WINDOW, wxCommandEvent );
wxDEFINE_EVENT( EVT_SEND_FILES, gui::ArrayEvent );

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
    if ( format != wxIPCFormat::wxIPC_UNICODETEXT )
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
    wxString dataStr = wxString( (wchar_t*)data, size / sizeof( wchar_t ) );

    if ( dataStr == DDEConsts::COMMAND_OPEN )
    {
        processOpen( dataStr );
    }
    else if ( dataStr.StartsWith( DDEConsts::COMMAND_SEND ) )
    {
        processSend( dataStr );
    }
}

void WinpinatorDDEServer::processOpen( const wxString& data )
{
    // Open the main app window
    wxCommandEvent openEv( EVT_OPEN_APP_WINDOW );
    wxPostEvent( m_handler, openEv );
}

void WinpinatorDDEServer::processSend( const wxString& data )
{
    wxString pathPart = data.Mid( DDEConsts::COMMAND_SEND.length() + 1 );

    wxArrayString paths = wxSplit( pathPart, '\0', '\0' );
    std::vector<wxString> pathVec;

    for ( const wxString& path : paths )
    {
        pathVec.push_back( path );
    }

    gui::ArrayEvent evnt( EVT_SEND_FILES );
    evnt.setArray( pathVec );
    wxPostEvent( m_handler, evnt );
}

