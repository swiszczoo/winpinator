#include "transfer_manager.hpp"

#include <wx/filename.h>

namespace srv
{

TransferManager::TransferManager( ObservableService* service )
    : m_running( ATOMIC_VAR_INIT( true ) )
    , m_srv( service )
    , m_lastId( 0 )
{
}

TransferManager::~TransferManager()
{
    if ( m_running )
    {
        stop();
    }
}

void TransferManager::stop()
{
    std::lock_guard<std::mutex> guard( m_mtx );
}

void TransferManager::registerTransfer( const std::string& remoteId,
    TransferOp& transfer )
{
    std::lock_guard<std::mutex> guard( m_mtx );

    transfer.id = m_lastId;
    m_lastId++;

    checkTransferDiskSpace( transfer );
    checkTransferMustOverwrite( transfer );
    setTransferTimestamp( transfer );

    m_transfers[remoteId].push_back( transfer );

    m_srv->notifyObservers( [this, &transfer]( srv::IServiceObserver* observer )
        { observer->onAddTransfer( transfer ); } );
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

void TransferManager::checkTransferDiskSpace( TransferOp& op )
{
    wxDiskspaceSize_t totalSpace;
    wxDiskspaceSize_t freeSpace;

    wxGetDiskSpace( m_outputPath, &totalSpace, &freeSpace );

    op.meta.notEnoughSpace = freeSpace < op.totalSize;
}

void TransferManager::checkTransferMustOverwrite( TransferOp& op )
{
    bool mustOverwrite = false;

    for ( std::string elementUtf8 : op.topDirBasenamesUtf8 )
    {
        wxString element = wxString::FromUTF8( elementUtf8 );

        wxFileName fname( m_outputPath, element );

        if ( fname.Exists() )
        {
            mustOverwrite = true;
            break;
        }
    }

    op.meta.mustOverwrite = mustOverwrite;
}

void TransferManager::setTransferTimestamp(TransferOp& op)
{
    op.meta.localTimestamp = time( NULL );
}

};
