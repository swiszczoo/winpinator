#pragma once
#include "observable_service.hpp"
#include "remote_info.hpp"
#include "transfer_types.hpp"

#include <wx/string.h>

#include <string>
#include <vector>

namespace srv
{

class ObservableService;

class IServiceObserver
{
    friend class ObservableService;

public:
    IServiceObserver();
    ~IServiceObserver();

    // Listeners to be overriden (must be virtual)
    virtual void onStateChanged() = 0;
    virtual void onIpAddressChanged( std::string newIp );

    virtual void onAddHost( srv::RemoteInfoPtr info );
    virtual void onEditHost( srv::RemoteInfoPtr newInfo );
    virtual void onDeleteHost( const std::string& id );

    virtual void onHostCountChanged( size_t newCount );

    virtual void onAddTransfer( std::string remoteId,
        srv::TransferOpPub transfer );
    virtual void onUpdateTransfer( std::string remoteId,
        srv::TransferOpPub transfer );
    virtual void onRemoveTransfer( std::string remoteId, int transferId );

    virtual void onOpenTransferUI( wxString remoteId );

protected:
    void observeService( ObservableService* service );

private:
    std::vector<ObservableService*> m_observedServices;

    void stopObserving( ObservableService* service );
};

};
