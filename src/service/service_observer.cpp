#include "service_observer.hpp"

namespace srv
{

IServiceObserver::IServiceObserver()
    : m_observedServices( {} )
{
}

IServiceObserver::~IServiceObserver()
{
    // Ensure to unregister from all currently observed services

    for ( ObservableService* serv : m_observedServices )
    {
        serv->removeObserver( this );
    }
}

void IServiceObserver::observeService( ObservableService* service )
{
    service->addObserver( this );
}

void IServiceObserver::stopObserving( ObservableService* service )
{
    size_t count = m_observedServices.size();

    for ( size_t i = 0; i < count; i++ )
    {
        if ( m_observedServices[i] == service )
        {
            m_observedServices.erase( m_observedServices.begin() + i );

            i--;
            count--;
        }
    }
}

};
