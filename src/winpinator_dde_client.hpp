#pragma once

#include <wx/wx.h>
#include <wx/dde.h>

#include <memory>

class WinpinatorDDEClient
{
public:
    WinpinatorDDEClient();

    bool isConnected() const;

    void execOpen();
    void execSend( const std::vector<wxString>& paths );

private:
    std::unique_ptr<wxDDEClient> m_client;
    wxDDEConnection* m_connection;
};
