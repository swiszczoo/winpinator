#pragma once
#include "observable_service.hpp"

#include "../settings_model.hpp"
#include "../zeroconf/mdns_types.hpp"
#include "database_manager.hpp"
#include "event.hpp"
#include "file_crawler.hpp"
#include "remote_manager.hpp"
#include "service_errors.hpp"
#include "transfer_manager.hpp"

#include <wintoast/wintoastlib.h>

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

    bool isServiceReady() const;
    bool isOnline() const;
    std::string getIpAddress();
    const std::string& getDisplayName() const;

    bool hasError() const;
    ServiceError getError() const;

    RemoteManager* getRemoteManager() const;
    TransferManager* getTransferManager() const;
    DatabaseManager* getDb() const;

    int startOnThisThread( const SettingsModel& appSettings );

    void postEvent( const Event& evnt );

private:
    const static std::string SERVICE_TYPE;
    const static int s_pollingIntervalSec;

    std::recursive_mutex m_mutex;

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
    std::shared_ptr<FileCrawler> m_crawler;

    SettingsModel m_settings;

    void initDatabase();
    void serviceMain();

    void notifyStateChanged();
    void notifyIpChanged();

    void onServiceAdded( const zc::MdnsServiceData& serviceData );
    void onServiceRemoved( const std::string& serviceName );

    int networkPollingMain( std::mutex& mtx, std::condition_variable& condVar );
};

};
