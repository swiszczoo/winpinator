#pragma once
#include "observable_service.hpp"

#include "../zeroconf/mdns_types.hpp"
#include "database_manager.hpp"
#include "event.hpp"
#include "remote_manager.hpp"
#include "service_errors.hpp"
#include "transfer_manager.hpp"

#include <wx/wx.h>
#include <wx/msgqueue.h>

#include <atomic>
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

    bool hasError() const;
    ServiceError getError() const;

    RemoteManager* getRemoteManager() const;
    TransferManager* getTransferManager() const;
    DatabaseManager* getDb() const;

    int startOnThisThread();

    void postEvent( const Event& evnt );

private:
    const static std::string SERVICE_TYPE;
    const static int s_pollingIntervalSec;

    std::recursive_mutex m_mutex;

    uint16_t m_port;
    uint16_t m_authPort;
    bool m_ready;
    bool m_online;
    bool m_shouldRestart;
    bool m_stopping;

    std::atomic<ServiceError> m_error;

    std::string m_ip;
    std::string m_displayName;

    std::thread m_pollingThread;
    wxMessageQueue<Event> m_events;
    std::shared_ptr<RemoteManager> m_remoteMgr;
    std::shared_ptr<TransferManager> m_transferMgr;
    std::shared_ptr<DatabaseManager> m_db;

    void initDatabase();
    void serviceMain();

    void notifyStateChanged();
    void notifyIpChanged();

    void onServiceAdded( const zc::MdnsServiceData& serviceData );
    void onServiceRemoved( const std::string& serviceName );

    int networkPollingMain( std::mutex& mtx, std::condition_variable& condVar );
};

};
