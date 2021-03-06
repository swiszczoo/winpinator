#pragma once
#include "observable_service.hpp"
#include "remote_info.hpp"
#include "transfer_manager.hpp"

#include <shared_mutex>

namespace srv
{

class TransferManager;

class RemoteManager
{
public:
    RemoteManager( ObservableService* service );
    ~RemoteManager();

    void setTransferManager( TransferManager* mgr );

    void stop();

    size_t getTotalHostsCount();
    size_t getVisibleHostsCount();

    std::vector<RemoteInfoPtr> generateCurrentHostList();

    void setServiceType( const std::string& serviceType );
    const std::string& getServiceType();

    void processAddHost( const zc::MdnsServiceData& data );
    void processRemoveHost( const std::string& id );

    bool isHostAvailable( const std::string& id, bool strip = false );
    std::shared_ptr<RemoteInfo> getRemoteInfo( const std::string& id, 
        bool strip = false );

private:
    static const std::string FALLBACK_OS;
    static const std::string REQUEST;

    std::shared_mutex m_mutex;
    std::vector<std::shared_ptr<RemoteInfo>> m_hosts;
    std::set<std::pair<std::string, zc::MdnsIpPair>> m_hostSet;
    std::string m_srvType;

    ObservableService* m_srv;
    TransferManager* m_transferMgr;

    std::string stripServiceFromIdent( const std::string& identStr ) const;
    int remoteThreadEntry( std::shared_ptr<RemoteInfo> serviceInfo );

    bool doRegistrationV1( RemoteInfo* serviceInfo );
    bool doRegistrationV2( RemoteInfo* serviceInfo );

    size_t _getVisibleHostsCount();
};

};
