#pragma once
#include "observable_service.hpp"

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
    virtual void onAddHost( int index, const std::string& id );
    virtual void onEditHost( int index, const std::string& id );
    virtual void onDeleteHost( const std::string& id );

protected:
    void observeService( ObservableService* service );

private:
    std::vector<ObservableService*> m_observedServices;

    void stopObserving( ObservableService* service );
};

};
