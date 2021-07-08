#pragma once
#include "observable_service.hpp"

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

protected:
    void observeService( ObservableService* service );

private:
    std::vector<ObservableService*> m_observedServices;

    void stopObserving( ObservableService* service );
};

};
