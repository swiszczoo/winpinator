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

private:
    std::unique_ptr<wxDDEClient> m_client;
    wxDDEConnection* m_connection;
};
