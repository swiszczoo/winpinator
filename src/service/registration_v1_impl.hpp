#pragma once
#include <wx/wx.h>
#include <wx/socket.h>

#include <memory>
#include <string>
#include <thread>

namespace srv
{

class RegistrationV1Server
{
public:
    RegistrationV1Server( std::string address, uint16_t port );

    bool startServer();
    bool stopServer();

private:
    static const std::string REQUEST;

    std::thread m_thread;
    std::unique_ptr<wxDatagramSocket> m_sock;
    std::string m_ipAddr;
    uint16_t m_port;
    bool m_exiting;

    int threadMain();
};

};
