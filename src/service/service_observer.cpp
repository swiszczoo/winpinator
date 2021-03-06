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

    m_observedServices.push_back( service );
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

//
// All the methods below are default stub implementations
// They should be selectively overriden by interested observers

void IServiceObserver::onIpAddressChanged( std::string newIp )
{
}

void IServiceObserver::onAddHost( srv::RemoteInfoPtr info )
{
}

void IServiceObserver::onEditHost( srv::RemoteInfoPtr info )
{
}

void IServiceObserver::onDeleteHost( const std::string& id )
{
}

void IServiceObserver::onHostCountChanged( size_t newCount )
{
}

void IServiceObserver::onAddTransfer( std::string remoteId, 
    srv::TransferOpPub transfer )
{
}

void IServiceObserver::onUpdateTransfer( std::string remoteId, 
    srv::TransferOpPub transfer )
{
}

void IServiceObserver::onRemoveTransfer( std::string remoteId, int transferId )
{
}

void IServiceObserver::onOpenTransferUI( wxString remoteId )
{
}


};
