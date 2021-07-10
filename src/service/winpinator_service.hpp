#pragma once
#include "observable_service.hpp"

#include "event.hpp"

#include <wx/wx.h>
#include <wx/msgqueue.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

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
    bool isOnline() const;

    int startOnThisThread();

private:
    const static std::string s_warpServiceType;
    const static int s_pollingIntervalSec;

    std::recursive_mutex m_mutex;

    uint16_t m_port;
    uint16_t m_authPort;
    bool m_ready;
    bool m_online;
    bool m_shouldRestart;
    bool m_stopping;

    std::thread m_pollingThread;
    wxMessageQueue<Event> m_events;

    void serviceMain();

    void notifyStateChanged();

    int networkPollingMain( std::mutex& mtx, std::condition_variable& condVar );
};

};
