#include "transfer_manager.hpp"

#include "../globals.hpp"
#include "notification_accept_files.hpp"

#include <wx/filename.h>

#include <memory>

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

void TransferManager::setOutputPath( const std::wstring& path )
{
    std::lock_guard<std::mutex> guard( m_mtx );

    m_outputPath = path;
}

std::wstring TransferManager::getOutputPath()
{
    std::lock_guard<std::mutex> guard( m_mtx );

    return m_outputPath;
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
    sendNotifications( remoteId, transfer );

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

void TransferManager::setTransferTimestamp( TransferOp& op )
{
    op.meta.localTimestamp = time( NULL );
}

void TransferManager::sendNotifications( const std::string& remoteId, 
    TransferOp& op )
{
    if ( !op.outcoming )
    {
        // TODO: add failure case notification

        auto notif = std::make_shared<AcceptFilesNotification>( remoteId, op.id );
        notif->setSenderFullName( wxString::FromUTF8( op.senderNameUtf8 ) );
        notif->setElementCount( op.topDirBasenamesUtf8.size() );
        notif->setIsSingleFolder( op.mimeIfSingleUtf8 == "inode/directory" );
        if ( op.topDirBasenamesUtf8.size() == 1 )
        {
            notif->setSingleElementName( 
                wxString::FromUTF8( op.topDirBasenamesUtf8[0] ) );
        }

        Event evnt;
        evnt.type = EventType::SHOW_TOAST_NOTIFICATION;
        evnt.eventData.toastData = notif;
        
        Globals::get()->getWinpinatorServiceInstance()->postEvent( evnt );
    }
}

};
