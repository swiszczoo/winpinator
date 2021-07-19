#pragma once
#include "../zeroconf/mdns_types.hpp"
#include "observable_service.hpp"

#include <condition_variable>
#include <memory>
#include <mutex>
#include <vector>

namespace srv
{

struct RemoteInfo
{
    std::string id;

    // mDNS metadata
    zc::MdnsIpPair ips;
    int port;
    int authPort;
    int apiVersion;
    std::string os;
    std::string hostname;

    std::thread thread;
    std::mutex mutex;
    std::condition_variable stopVar;
    bool stopping;

    // Host should be visible after its public key has been successfully
    // decoded using key group
    bool visible; 
};

class RemoteManager
{
public:
    RemoteManager( ObservableService* service );
    ~RemoteManager();

    void stop();

    size_t getTotalHostsCount();
    size_t getVisibleHostsCount();

    void setServiceType( const std::string& serviceType );
    const std::string& getServiceType();

    void processAddHost( const zc::MdnsServiceData& data );
    void processRemoveHost( const std::string& id );

private:
    static const std::string FALLBACK_OS;
    static const std::string REQUEST;

    std::mutex m_mutex;
    std::vector<std::shared_ptr<RemoteInfo>> m_hosts;
    std::string m_srvType;

    ObservableService* m_srv;

    std::string stripServiceFromIdent( const std::string& identStr ) const;
    int remoteThreadEntry( std::shared_ptr<RemoteInfo> serviceInfo );

    bool doRegistrationV1( RemoteInfo* serviceInfo );
    bool doRegistrationV2( RemoteInfo* serviceInfo );
};

};
