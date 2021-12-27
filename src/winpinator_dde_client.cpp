#include "winpinator_dde_client.hpp"
#include "winpinator_dde_consts.hpp"

WinpinatorDDEClient::WinpinatorDDEClient()
    : m_client( nullptr )
{
    m_client = std::make_unique<wxDDEClient>();
    m_connection = static_cast<wxDDEConnection*>( m_client->MakeConnection(
        wxEmptyString, DDEConsts::SERVICE_NAME, DDEConsts::TOPIC_NAME ) );
}

bool WinpinatorDDEClient::isConnected() const
{
    return m_connection != nullptr;
}

void WinpinatorDDEClient::execOpen()
{
    if ( !m_connection )
    {
        return;
    }

    std::wstring data = DDEConsts::COMMAND_OPEN.ToStdWstring();
    m_connection->Execute( data.data(),
        data.size() * sizeof( wchar_t ), wxIPC_UNICODETEXT );
}

void WinpinatorDDEClient::execSend( const std::vector<wxString>& paths )
{
    if ( !m_connection )
    {
        return;
    }

    std::wstring data = DDEConsts::COMMAND_SEND.ToStdWstring();
    data += ' ';

    for ( const wxString& path : paths )
    {
        data += path.ToStdWstring();
        data += L'\0';
    }

    // Decrease the length by 1 character to ignore the last NUL
    m_connection->Execute( data.data(),
        ( data.size() - 1 ) * sizeof( wchar_t ), wxIPC_UNICODETEXT );
}
