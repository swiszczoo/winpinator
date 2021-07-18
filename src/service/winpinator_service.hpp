#pragma once
#include "observable_service.hpp"

#include "../zeroconf/mdns_types.hpp"
#include "event.hpp"

#include <wx/wx.h>
#include <wx/msgqueue.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
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
    std::string getIpAddress();
    const std::string& getDisplayName() const;

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

    std::string m_ip;
    std::string m_displayName;

    std::thread m_pollingThread;
    wxMessageQueue<Event> m_events;

    void serviceMain();

    void notifyStateChanged();
    void notifyIpChanged();

    void onServiceAdded( const zc::MdnsServiceData& serviceData );
    void onServiceRemoved( const std::string& serviceName );

    int networkPollingMain( std::mutex& mtx, std::condition_variable& condVar );
};

};
