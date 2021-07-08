#pragma once
#include "observable_service.hpp"

#include <cstdint>
#include <mutex>
#include <functional>

namespace srv
{

class WinpinatorService : public ObservableService
{
public:
    WinpinatorService();

    void setGrpcPort( uint16_t port );
    uint16_t getGrpcPort() const;

    void setAuthPort( uint16_t authPort );
    uint16_t getAuthPort() const;

    bool isServiceReady() const;

    int startOnThisThread();

private:
    const static std::string s_warpServiceType;

    std::recursive_mutex m_mutex;

    uint16_t m_port;
    uint16_t m_authPort;
    bool m_ready;

    void notifyStateChanged();
};

};
