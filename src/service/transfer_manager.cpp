#include "transfer_manager.hpp"

namespace srv
{

TransferManager::TransferManager( ObservableService* service )
    : m_srv( service )
    , m_lastId( 0 )
{
}

void TransferManager::stop()
{
}

void TransferManager::registerTransfer( const std::string& remoteId,
    TransferOp& transfer )
{
    std::lock_guard<std::mutex> guard( m_mtx );

    transfer.id = m_lastId;
    m_lastId++;

    m_transfers[remoteId].push_back( transfer );

    m_srv->notifyObservers( [this, transfer ]( srv::IServiceObserver* observer ) { 
        observer->onAddTransfer( transfer );
    } );
}

void TransferManager::replyAllowTransfer( const std::string& remoteId,
    int transferId, bool allow )
{
    std::lock_guard<std::mutex> guard( m_mtx );
}

std::vector<TransferOp> TransferManager::getTransfersForRemote(
    const std::string& remoteId )
{
    std::lock_guard<std::mutex> guard( m_mtx );

    if ( m_transfers.find( remoteId ) != m_transfers.end() )
    {
        return m_transfers[remoteId];
    }

    return {};
}

};
