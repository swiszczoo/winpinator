#include "transfer_manager.hpp"

#include "../globals.hpp"
#include "auth_manager.hpp"
#include "file_sender.hpp"
#include "notification_accept_files.hpp"
#include "notification_transfer_failed.hpp"
#include "service_utils.hpp"

#include <wx/filename.h>
#include <wx/mimetype.h>

#include <memory>

namespace srv
{

const long long TransferManager::PROGRESS_FREQ_MILLIS = 250;
const std::string TransferManager::DEFAULT_MIME_TYPE = "application/octet-stream";
const std::string TransferManager::DIRECTORY_MIME_TYPE = "inode/directory";

TransferManager::TransferManager( ObservableService* service )
    : m_running( ATOMIC_VAR_INIT( true ) )
    , m_remoteMgr( nullptr )
    , m_srv( service )
    , m_lastId( 0 )
    , m_outputPath( L"" )
    , m_compressionLevel( 0 )
    , m_mustAllowIncoming( true )
    , m_mustAllowOverwrite( true )
    , m_filePerms( 0 )
    , m_execPerms( 0 )
    , m_dirPerms( 0 )
    , m_empty()
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

void TransferManager::setRemoteManager( std::shared_ptr<RemoteManager> ptr )
{
    std::lock_guard<std::mutex> guard( m_mtx );

    m_remoteMgr = ptr;
}

RemoteManager* TransferManager::getRemoteManager()
{
    std::lock_guard<std::mutex> guard( m_mtx );

    return m_remoteMgr.get();
}

void TransferManager::setDatabaseManager( std::shared_ptr<DatabaseManager> ptr )
{
    std::lock_guard<std::mutex> guard( m_mtx );

    m_dbMgr = ptr;
}

DatabaseManager* TransferManager::getDatabaseManager()
{
    std::lock_guard<std::mutex> guard( m_mtx );

    return m_dbMgr.get();
}

void TransferManager::setCrawlerPtr( std::shared_ptr<FileCrawler> ptr )
{
    m_crawler = ptr;
}

FileCrawler* TransferManager::getFileCrawler()
{
    return m_crawler.get();
}

void TransferManager::setCompressionLevel( int level )
{
    std::lock_guard<std::mutex> guard( m_mtx );

    m_compressionLevel = level;
}

int TransferManager::getCompressionLevel()
{
    std::lock_guard<std::mutex> guard( m_mtx );

    return m_compressionLevel;
}

void TransferManager::setMustAllowIncoming( bool must )
{
    std::lock_guard<std::mutex> guard( m_mtx );

    m_mustAllowIncoming = must;
}

bool TransferManager::getMustAllowIncoming()
{
    std::lock_guard<std::mutex> guard( m_mtx );

    return m_mustAllowIncoming;
}

void TransferManager::setMustAllowOverwrite( bool must )
{
    std::lock_guard<std::mutex> guard( m_mtx );

    m_mustAllowOverwrite = must;
}

bool TransferManager::getMustAllowOverwrite()
{
    std::lock_guard<std::mutex> guard( m_mtx );

    return m_mustAllowOverwrite;
}

void TransferManager::setUnixPermissionMasks( int file,
    int executable, int directory )
{
    std::lock_guard<std::mutex> guard( m_mtx );

    m_filePerms = file;
    m_execPerms = executable;
    m_dirPerms = directory;
}

int TransferManager::getUnixFilePermissionMask()
{
    std::lock_guard<std::mutex> guard( m_mtx );

    return m_filePerms;
}

int TransferManager::getUnixExecutablePermissionMask()
{
    std::lock_guard<std::mutex> guard( m_mtx );

    return m_execPerms;
}

int TransferManager::getUnixDirectoryPermissionMask()
{
    std::lock_guard<std::mutex> guard( m_mtx );

    return m_dirPerms;
}

void TransferManager::stop()
{
    std::lock_guard<std::mutex> guard( m_mtx );
}

TransferOpPtr TransferManager::registerTransfer( const std::string& remoteId,
    TransferOp& transfer, bool firstTry )
{
    std::lock_guard<std::mutex> guard( m_mtx );
    TransferOpPtr opPtr = nullptr;

    if ( firstTry )
    {
        transfer.id = m_lastId;
        transfer.mutex = std::make_shared<std::mutex>();
        m_lastId++;
    }

    bool autostart = false;

    {
        std::lock_guard<std::mutex> lock( *transfer.mutex );

        transfer.meta.sentBytes = 0;

        transfer.intern.fileCount = 0;
        transfer.intern.dirCount = 0;
        transfer.intern.remoteId = remoteId;

        checkTransferDiskSpace( transfer );
        checkTransferMustOverwrite( transfer );
        setTransferTimestamp( transfer );
        setUpPauseLock( transfer );

        if ( !transfer.outcoming )
        {
            if ( !m_mustAllowIncoming )
            {
                autostart = true;
            }
            else if ( !m_mustAllowOverwrite && transfer.meta.mustOverwrite )
            {
                autostart = true;
            }
        }

        sendNotifications( remoteId, transfer, !autostart );

        if ( firstTry )
        {
            m_transfers[remoteId].push_back(
                opPtr = std::make_shared<TransferOp>( transfer ) );
        }
    }

    if ( autostart )
    {
        doReplyAllowTransfer( remoteId, transfer.id, true );
    }

    std::lock_guard<std::mutex> lock( *transfer.mutex );
    m_srv->notifyObservers(
        [this, &remoteId, &transfer]( srv::IServiceObserver* observer )
        { observer->onAddTransfer( remoteId, transfer ); } );

    return opPtr;
}

void TransferManager::replyAllowTransfer( const std::string& remoteId,
    int transferId, bool allow )
{
    std::lock_guard<std::mutex> guard( m_mtx );

    doReplyAllowTransfer( remoteId, transferId, allow );
}

void TransferManager::cancelTransferRequest( const std::string& remoteId,
    time_t transferId )
{
    std::lock_guard<std::mutex> guard( m_mtx );

    TransferOpPtr op = getTransferByTimestamp( remoteId, transferId );
    if ( !op )
    {
        return;
    }

    {
        std::lock_guard<std::mutex> opLock( *op->mutex );
        if ( op->outcoming )
        {
            op->status = OpStatus::CANCELLED_PERMISSION_BY_RECEIVER;
        }
        else
        {
            op->status = OpStatus::CANCELLED_PERMISSION_BY_SENDER;
        }
    }

    doFinishTransfer( remoteId, op->id );
}

void TransferManager::resumeTransfer( const std::string& remoteId,
    int transferId )
{
    std::lock_guard<std::mutex> guard( m_mtx );

    TransferOpPtr op = getTransferInfo( remoteId, transferId );
    bool opPaused = false;

    {
        std::lock_guard<std::mutex> lck( *op->mutex );
        opPaused = op->status == OpStatus::PAUSED;
    }

    if ( !op || !opPaused )
    {
        return;
    }

    {
        std::lock_guard<std::mutex> lck( op->intern.pauseLock->mutex );
        op->intern.pauseLock->value = false;
        op->intern.pauseLock->condVar.notify_all();
    }

    {
        std::lock_guard<std::mutex> lck( *op->mutex );
        op->status = OpStatus::TRANSFERRING;

        sendStatusUpdateNotification( remoteId, op );
    }
}

void TransferManager::pauseTransfer( const std::string& remoteId,
    int transferId )
{
    std::lock_guard<std::mutex> guard( m_mtx );

    TransferOpPtr op = getTransferInfo( remoteId, transferId );
    bool opRunning = false;

    if ( !op )
    {
        return;
    }

    {
        std::lock_guard<std::mutex> lck( *op->mutex );
        opRunning = op->status == OpStatus::TRANSFERRING;
    }

    if ( !opRunning )
    {
        return;
    }

    {
        std::lock_guard<std::mutex> lck( op->intern.pauseLock->mutex );
        op->intern.pauseLock->value = true;
        op->intern.pauseLock->condVar.notify_all();
    }

    {
        std::lock_guard<std::mutex> lck( *op->mutex );
        op->status = OpStatus::PAUSED;

        sendStatusUpdateNotification( remoteId, op );
    }
}

void TransferManager::stopTransfer( const std::string& remoteId,
    int transferId, bool error )
{
    std::lock_guard<std::mutex> guard( m_mtx );

    TransferOpPtr op = getTransferInfo( remoteId, transferId );

    if ( !op )
    {
        return;
    }

    {
        std::lock_guard<std::mutex> opLock( *op->mutex );
        if ( op->outcoming )
        {
            if ( error )
            {
                op->status = OpStatus::FAILED;
                sendFailureNotification( remoteId,
                    wxString::FromUTF8( op->receiverNameUtf8 ).ToStdWstring() );
            }
            else
            {
                op->status = OpStatus::STOPPED_BY_RECEIVER;
            }
        }
        else
        {
            if ( error )
            {
                op->status = OpStatus::FAILED;
                sendFailureNotification( remoteId,
                    wxString::FromUTF8( op->senderNameUtf8 ).ToStdWstring() );
            }
            else
            {
                op->status = OpStatus::STOPPED_BY_SENDER;
            }
        }
    }

    doFinishTransfer( remoteId, op->id );
}

void TransferManager::finishTransfer( const std::string& remoteId,
    int transferId )
{
    std::lock_guard<std::mutex> guard( m_mtx );
    doFinishTransfer( remoteId, transferId );
}

void TransferManager::requestStopTransfer( const std::string& remoteId,
    int transferId, bool error )
{
    std::lock_guard<std::mutex> guard( m_mtx );

    TransferOpPtr op = getTransferInfo( remoteId, transferId );

    if ( !op )
    {
        return;
    }

    RemoteInfoPtr info = m_remoteMgr->getRemoteInfo( remoteId );

    if ( !info )
    {
        wxLogDebug( "TransferManager: remote not found! ignoring!" );
        return;
    }

    std::unique_lock<std::mutex> lock( info->mutex );

    grpc::ClientContext ctx;
    ctx.set_deadline( std::chrono::system_clock::now()
        + std::chrono::seconds( 3 ) );
    StopInfo request;
    OpInfo* opInfo = new OpInfo( convertOpToOpInfo( op, m_compressionLevel > 0 ) );
    request.set_allocated_info( opInfo );
    request.set_error( error );

    VoidType response;

    if ( !info->stub->StopTransfer( &ctx, request, &response ).ok() )
    {
        wxLogDebug( "TransferManager: StopTransfer failed" );
    }
    else
    {
        lock.unlock();

        {
            std::lock_guard<std::mutex> opLock( *op->mutex );
            if ( error )
            {
                op->status = OpStatus::FAILED;
            }
            else if ( op->outcoming )
            {
                op->status = OpStatus::STOPPED_BY_SENDER;
            }
            else
            {
                op->status = OpStatus::STOPPED_BY_RECEIVER;
            }
        }

        doFinishTransfer( remoteId, transferId );
    }
}

bool TransferManager::handleOutcomingTransfer( const std::string& remoteId,
    int transferId, grpc::ServerWriter<FileChunk>* writer, bool compress )
{
    TransferOpPtr op;
    {
        std::lock_guard<std::mutex> guard( m_mtx );
        op = getTransferInfo( remoteId, transferId );
    }

    if ( !op )
    {
        return false;
    }

    {
        std::lock_guard<std::mutex> lck( *op->mutex );
        op->status = OpStatus::TRANSFERRING;

        sendStatusUpdateNotification( remoteId, op );
    }

    FileSender sender( op, writer );

    {
        std::lock_guard<std::mutex> guard( m_mtx );
        sender.setCompressionLevel( compress ? m_compressionLevel : 0 );
        sender.setUnixPermissionMasks( m_filePerms, m_execPerms, m_dirPerms );
    }

    bool result = sender.transferFiles( [&]()
        { sendStatusUpdateNotification( remoteId, op ); } );

    {
        std::lock_guard<std::mutex> guard( *op->mutex );
        if ( result )
        {
            op->status = OpStatus::FINISHED;
        }
        else if ( op->status == OpStatus::TRANSFERRING 
            || op->status == OpStatus::PAUSED )
        {
            op->status = OpStatus::FAILED_UNRECOVERABLE;
        }
    }

    doFinishTransfer( remoteId, op->id );

    return result;
}

void TransferManager::failAll( const std::string& remoteId )
{
    std::lock_guard<std::mutex> guard( m_mtx );

    bool someFailed = false;
    std::wstring senderName;
    if ( m_transfers.find( remoteId ) != m_transfers.end() )
    {
        while ( !m_transfers[remoteId].empty() )
        {
            TransferOpPtr op = *m_transfers[remoteId].begin();
            failOp( op );

            if ( senderName.empty() )
            {
                if ( op->outcoming )
                {
                    RemoteInfoPtr info = m_remoteMgr->getRemoteInfo( remoteId );
                    senderName = info->fullName;
                }
                else
                {
                    senderName = wxString::FromUTF8(
                        op->senderNameUtf8 )
                                     .ToStdWstring();
                }
            }
        }
    }

    if ( someFailed )
    {
        sendFailureNotification( remoteId, senderName );
    }
}

int TransferManager::createOutcomingTransfer( const std::string& remoteId,
    const std::vector<std::wstring>& rootPaths )
{
    int folders = 0;
    int files = 0;
    std::wstring oneFileName;

    for ( auto path : rootPaths )
    {
        if ( wxDirExists( path ) )
        {
            folders++;
            oneFileName = path;
        }
        else if ( wxFileExists( path ) )
        {
            files++;
            oneFileName = path;
        }
    }

    TransferOp op;
    op.outcoming = true;
    op.status = OpStatus::CALCULATING;
    op.startTime = Utils::getMonotonicTime();
    op.mimeIfSingleUtf8 = DEFAULT_MIME_TYPE;
    op.senderNameUtf8 = wxString( Utils::getUserFullName() ).ToUTF8();
    op.receiverUtf8 = AuthManager::get()->getIdent();
    op.receiverNameUtf8 = "";
    op.useCompression = m_compressionLevel > 0;

    if ( files == 0 && folders == 1 )
    {
        op.mimeIfSingleUtf8 = DIRECTORY_MIME_TYPE;

        wxFileName dirName( oneFileName );
        op.nameIfSingleUtf8 = dirName.GetFullName().ToUTF8();
    }
    else if ( files == 1 && folders == 0 )
    {
        wxFileName fname( oneFileName );
        wxFileType* ftype = wxTheMimeTypesManager->GetFileTypeFromExtension(
            fname.GetExt() );

        op.nameIfSingleUtf8 = fname.GetFullName().ToUTF8();

        if ( ftype )
        {
            wxString mime;
            if ( ftype->GetMimeType( &mime ) )
            {
                op.mimeIfSingleUtf8 = mime.ToUTF8();
            }

            delete ftype;
        }
    }
    else
    {
        op.nameIfSingleUtf8 = wxString::Format(
            "%d elements", (int)( folders + files ) )
                                  .ToUTF8();
    }

    op.totalSize = -1;
    op.totalCount = rootPaths.size();

    // Setup topDirBaseNames (temporarily)
    for ( const std::wstring& path : rootPaths )
    {
        wxFileName fname( path );
        op.topDirBasenamesUtf8.push_back(
            std::string( fname.GetFullName().ToUTF8() ) );
    }

    TransferOpPtr opPtr = registerTransfer( remoteId, op, true );
    opPtr->intern.crawlJobId = m_crawler->startCrawlJob( rootPaths );
    m_crawlJobs[opPtr->intern.crawlJobId] = opPtr;

    return op.id;
}

void TransferManager::notifyCrawlerSucceeded( const CrawlerOutputData& data )
{
    std::lock_guard<std::mutex> guard( m_mtx );
    m_crawler->releaseCrawlJob( data.jobId );

    if ( m_crawlJobs.find( data.jobId ) != m_crawlJobs.end() )
    {
        TransferOpPtr op = m_crawlJobs[data.jobId];
        doSendRequestAfterCrawling( op, data );
        m_crawlJobs.erase( data.jobId );
    }
}

void TransferManager::notifyCrawlerFailed( int jobId )
{
    std::lock_guard<std::mutex> guard( m_mtx );
    m_crawler->releaseCrawlJob( jobId );

    if ( m_crawlJobs.find( jobId ) != m_crawlJobs.end() )
    {
        TransferOpPtr failedOp = m_crawlJobs[jobId];
        failOp( failedOp );
        m_crawlJobs.erase( jobId );
    }
}

std::mutex& TransferManager::getMutex()
{
    return m_mtx;
}

std::vector<TransferOpPtr> TransferManager::getTransfersForRemote(
    const std::string& remoteId )
{
    std::lock_guard<std::mutex> guard( m_mtx );

    if ( m_transfers.find( remoteId ) != m_transfers.end() )
    {
        return m_transfers[remoteId];
    }

    return {};
}

TransferOpPub TransferManager::getOp( const std::string& remoteId,
    time_t timestamp )
{
    std::lock_guard<std::mutex> guard( m_mtx );

    TransferOpPtr ptr = getTransferByTimestamp( remoteId, timestamp );

    if ( !ptr )
    {
        TransferOpPub invalid;
        invalid.id = -1;

        return invalid;
    }

    std::lock_guard<std::mutex> opLock( *ptr->mutex );
    TransferOpPub op = *ptr;

    return op;
}

void TransferManager::checkTransferDiskSpace( TransferOp& op )
{
    if ( op.outcoming )
    {
        op.meta.notEnoughSpace = false;
    }
    else
    {
        wxDiskspaceSize_t totalSpace;
        wxDiskspaceSize_t freeSpace;

        wxGetDiskSpace( m_outputPath, &totalSpace, &freeSpace );

        op.meta.notEnoughSpace = freeSpace < op.totalSize;
    }
}

void TransferManager::checkTransferMustOverwrite( TransferOp& op )
{
    wxLogNull logNull;
    bool mustOverwrite = false;

    for ( std::string elementUtf8 : op.topDirBasenamesUtf8 )
    {
        wxString element = wxString::FromUTF8( elementUtf8 );

        if ( element.Contains( ":" ) )
        {
            continue;
        }

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
    op.intern.lastProgressUpdate = std::chrono::steady_clock::now();
}

void TransferManager::setUpPauseLock( TransferOp& op )
{
    op.intern.pauseLock = std::make_shared<EventLock>();
    op.intern.pauseLock->value = false;
}

void TransferManager::sendNotifications( const std::string& remoteId,
    TransferOp& op, bool actionRequired )
{
    if ( !op.outcoming )
    {
        auto notif = std::make_shared<AcceptFilesNotification>( remoteId, op.id );
        notif->setSenderFullName(
            wxString::FromUTF8( op.senderNameUtf8 ).ToStdWstring() );
        notif->setElementCount( op.topDirBasenamesUtf8.size() );
        notif->setIsSingleFolder( op.mimeIfSingleUtf8 == "inode/directory" );
        notif->setOverwriteNeeded( op.meta.mustOverwrite );
        notif->setShowActions( actionRequired );

        if ( op.topDirBasenamesUtf8.size() == 1 )
        {
            notif->setSingleElementName(
                wxString::FromUTF8( op.topDirBasenamesUtf8[0] ).ToStdWstring() );
        }

        Event evnt;
        evnt.type = EventType::SHOW_TOAST_NOTIFICATION;
        evnt.eventData.toastData = notif;

        Globals::get()->getWinpinatorServiceInstance()->postEvent( evnt );
    }
}

TransferOpPtr TransferManager::getTransferInfo( const std::string& remoteId,
    int transferId )
{
    if ( m_transfers.find( remoteId ) != m_transfers.end() )
    {
        auto& transferList = m_transfers[remoteId];

        for ( TransferOpPtr op : transferList )
        {
            if ( op->id == transferId )
            {
                return op;
            }
        }
    }

    return nullptr;
}

TransferOpPtr TransferManager::getTransferByTimestamp(
    const std::string& remoteId, time_t timestamp )
{
    if ( m_transfers.find( remoteId ) != m_transfers.end() )
    {
        auto& transferList = m_transfers[remoteId];

        for ( TransferOpPtr op : transferList )
        {
            if ( op->startTime == timestamp )
            {
                return op;
            }
        }
    }

    return nullptr;
}

void TransferManager::processStartTransfer( const std::string& remoteId,
    TransferOpPtr op )
{
    wxLogDebug( "TransferManager: processing start transfer" );

    {
        RemoteInfoPtr info = m_remoteMgr->getRemoteInfo( remoteId );

        if ( !info )
        {
            wxLogDebug( "TransferManager: remote not found! ignoring!" );
            return;
        }

        std::lock_guard<std::mutex> guard( info->mutex );

        std::shared_ptr<grpc::ClientContext> ctx
            = std::make_shared<grpc::ClientContext>();

        std::shared_ptr<OpInfo> request = std::make_shared<OpInfo>(
            convertOpToOpInfo( op, m_compressionLevel > 0 ) );

        std::shared_ptr<StartTransferReactor> reactor
            = std::make_shared<StartTransferReactor>();
        reactor->setInstance( reactor );
        reactor->setRefs( ctx, request );
        reactor->setTransferPtr( op );
        reactor->setRemoteId( remoteId );
        reactor->setManager( this );
        reactor->setCanOverwrite( op->meta.mustOverwrite );

        wxLogDebug( "TransferManager: starting transfer!" );

        info->stub->experimental_async()->StartTransfer( ctx.get(), request.get(),
            reactor.get() );
        reactor->start();
    }

    std::lock_guard<std::mutex> lck( *op->mutex );
    op->status = OpStatus::TRANSFERRING;
    sendStatusUpdateNotification( remoteId, op );
}

void TransferManager::processDeclineTransfer( const std::string& remoteId,
    TransferOpPtr op )
{
    wxLogDebug( "TransferManager: rejecting transfer" );

    {
        RemoteInfoPtr info = m_remoteMgr->getRemoteInfo( remoteId );

        if ( !info )
        {
            wxLogDebug( "TransferManager: remote not found! ignoring!" );
            return;
        }

        std::lock_guard<std::mutex> guard( info->mutex );

        grpc::ClientContext ctx;
        ctx.set_deadline( std::chrono::system_clock::now()
            + std::chrono::seconds( 3 ) );
        OpInfo request = convertOpToOpInfo( op, m_compressionLevel > 0 );
        VoidType response;

        if ( !info->stub->CancelTransferOpRequest( &ctx, request, &response ).ok() )
        {
            wxLogDebug( "TransferManager: CancelTransferOpRequest failed" );
        }
    }

    {
        std::lock_guard<std::mutex> lck( *op->mutex );
        if ( op->outcoming )
        {
            op->status = OpStatus::CANCELLED_PERMISSION_BY_SENDER;
        }
        else
        {
            op->status = OpStatus::CANCELLED_PERMISSION_BY_RECEIVER;
        }
    }

    doFinishTransfer( remoteId, op->id );
}

OpInfo TransferManager::convertOpToOpInfo( const TransferOpPtr op,
    bool compressionEnabled ) const
{
    std::lock_guard<std::mutex> guard( *op->mutex );

    OpInfo info;

    info.set_ident( AuthManager::get()->getIdent() );
    info.set_timestamp( op->startTime );
    info.set_readable_name( Utils::getHostname() );
    info.set_use_compression( op->useCompression && compressionEnabled );

    return info;
}

void TransferManager::sendStatusUpdateNotification( const std::string& remoteId,
    const TransferOpPtr op )
{
    m_srv->notifyObservers( [this, &remoteId, &op]( IServiceObserver* observer )
        { observer->onUpdateTransfer( remoteId, *op ); } );
}

void TransferManager::sendFailureNotification( const std::string& remoteId,
    const std::wstring& senderName )
{
    auto notif = std::make_shared<TransferFailedNotification>( remoteId );
    notif->setSenderFullName( senderName );

    Event evnt;
    evnt.type = EventType::SHOW_TOAST_NOTIFICATION;
    evnt.eventData.toastData = notif;

    Globals::get()->getWinpinatorServiceInstance()->postEvent( evnt );
}

void TransferManager::doReplyAllowTransfer( const std::string& remoteId,
    int transferId, bool allow )
{
    TransferOpPtr op = getTransferInfo( remoteId, transferId );

    if ( !op )
    {
        return;
    }

    std::unique_lock<std::mutex> lock( *op->mutex );
    if ( op->status == OpStatus::WAITING_PERMISSION )
    {
        if ( allow && !op->outcoming )
        {
            lock.unlock();
            processStartTransfer( remoteId, op );
        }
        else if ( !allow )
        {
            lock.unlock();
            processDeclineTransfer( remoteId, op );
        }
    }
}

void TransferManager::doFinishTransfer( const std::string& remoteId,
    int transferId )
{
    TransferOpPtr op = getTransferInfo( remoteId, transferId );

    if ( !op )
    {
        return;
    }

    db::Transfer record;
    {
        std::lock_guard<std::mutex> lock( *op->mutex );

        record.elements = op->intern.elements;
        record.fileCount = op->intern.fileCount;
        record.folderCount = op->intern.dirCount;
        record.outgoing = op->outcoming;
        if ( op->totalCount == 1 )
        {
            record.singleElementName
                = wxString::FromUTF8( op->nameIfSingleUtf8 ).ToStdWstring();
        }
        else if ( op->topDirBasenamesUtf8.size() == 1 )
        {
            record.singleElementName = wxString::FromUTF8(
                op->topDirBasenamesUtf8[0] )
                                           .ToStdWstring();
        }

        record.status = getOpStatus( op );
        record.transferType = getOpType( op );

        // Correct file/folder count and type
        // in case of unfinished incoming transfer
        if ( !record.outgoing && ( record.status != db::TransferStatus::SUCCEEDED ) )
        {
            record.transferType = db::TransferType::UNFINISHED_INCOMING;

            if ( op->mimeIfSingleUtf8 == "inode/directory" )
            {
                record.folderCount = op->topDirBasenamesUtf8.size();
                record.fileCount = 0;
            }
            else
            {
                record.folderCount = 0;
                record.fileCount = op->topDirBasenamesUtf8.size();
            }
        }

        record.targetId = wxString( remoteId ).ToStdWstring();
        record.totalSizeBytes = op->totalSize;
        record.transferTimestamp = op->meta.localTimestamp;
    }

    m_dbMgr->addTransfer( record );

    std::vector<TransferOpPtr>& transferArr = m_transfers[remoteId];
    for ( size_t i = 0; i < transferArr.size(); i++ )
    {
        if ( transferArr[i] == op )
        {
            transferArr.erase( transferArr.begin() + i );
            break;
        }
    }

    m_srv->notifyObservers(
        [this, &remoteId, &transferId]( srv::IServiceObserver* observer )
        { observer->onRemoveTransfer( remoteId, transferId ); } );
}

void TransferManager::doSendRequestAfterCrawling( TransferOpPtr op,
    const CrawlerOutputData& data )
{
    OpInfo* info;
    std::string senderName, receiverName, receiver, nameIfSingle, mimeIfSingle;

    {
        // Update op using data from crawler
        std::lock_guard<std::mutex> guard( *op->mutex );
        op->topDirBasenamesUtf8 = data.topDirBasenamesUtf8;
        op->totalSize = data.totalSize;
        op->totalCount = data.paths->size();
        op->intern.fileCount = data.fileCount;
        op->intern.dirCount = data.folderCount;
        op->intern.rootDir = data.rootDir;
        op->intern.relativePaths = data.paths;
        op->status = OpStatus::WAITING_PERMISSION;

        senderName = op->senderNameUtf8;
        receiverName = op->receiverNameUtf8;
        receiver = op->receiverUtf8;
        nameIfSingle = op->nameIfSingleUtf8;
        mimeIfSingle = op->mimeIfSingleUtf8;

        sendStatusUpdateNotification( op->intern.remoteId, op );
    }

    info = new OpInfo( convertOpToOpInfo( op, m_compressionLevel > 0 ) );

    RemoteInfoPtr remote = m_remoteMgr->getRemoteInfo( op->intern.remoteId );

    std::lock_guard<std::mutex> guard( remote->mutex );

    auto ctx = std::make_shared<grpc::ClientContext>();
    auto request = std::make_shared<TransferOpRequest>();
    request->set_allocated_info( info );
    request->set_sender_name( senderName );
    request->set_receiver_name( receiverName );
    request->set_receiver( receiver );
    request->set_size( data.totalSize );
    request->set_count( data.paths->size() );
    request->set_name_if_single( nameIfSingle );
    request->set_mime_if_single( mimeIfSingle );
    for ( auto& topDir : data.topDirBasenamesUtf8 )
    {
        request->add_top_dir_basenames( topDir );
    }

    auto response = std::make_shared<VoidType>();

    auto timePoint = std::chrono::system_clock::now()
        + std::chrono::seconds( 5 );
    ctx->set_deadline( timePoint );

    if ( remote->stub )
    {
        remote->stub->experimental_async()->ProcessTransferOpRequest(
            ctx.get(), request.get(), response.get(),
            [this, ctx, request, response]( grpc::Status status ) {

            } );
    }
    else
    {
        failOp( op );
    }
}

bool TransferManager::failOp( TransferOpPtr op )
{
    bool failed = false;

    {
        std::lock_guard<std::mutex> lock( *op->mutex );
        if ( op->status != OpStatus::FAILED )
        {
            failed = true;
        }

        op->status = OpStatus::FAILED;
    }

    doFinishTransfer( op->intern.remoteId, op->id );
    return failed;
}

db::TransferStatus TransferManager::getOpStatus( const TransferOpPtr op )
{
    if ( op->status == OpStatus::CANCELLED_PERMISSION_BY_RECEIVER
        || op->status == OpStatus::CANCELLED_PERMISSION_BY_SENDER
        || op->status == OpStatus::STOPPED_BY_RECEIVER
        || op->status == OpStatus::STOPPED_BY_SENDER )
    {
        return db::TransferStatus::CANCELLED;
    }
    else if ( op->status == OpStatus::FAILED
        || op->status == OpStatus::FAILED_UNRECOVERABLE
        || op->status == OpStatus::FILE_NOT_FOUND
        || op->status == OpStatus::INVALID_OP )
    {
        return db::TransferStatus::FAILED;
    }

    return db::TransferStatus::SUCCEEDED;
}

db::TransferType TransferManager::getOpType( const TransferOpPtr op )
{
    if ( op->intern.fileCount == 0 && op->intern.dirCount == 0 )
    {
        return db::TransferType::SINGLE_FILE;
    }

    if ( op->intern.fileCount == 0 )
    {
        if ( op->intern.dirCount == 1 )
        {
            return db::TransferType::SINGLE_DIRECTORY;
        }
        if ( op->intern.dirCount > 1 )
        {
            return db::TransferType::MULTIPLE_DIRECTORIES;
        }
    }
    if ( op->intern.dirCount == 0 )
    {
        if ( op->intern.fileCount == 1 )
        {
            return db::TransferType::SINGLE_FILE;
        }
        if ( op->intern.fileCount > 1 )
        {
            return db::TransferType::MULTIPLE_FILES;
        }
    }

    return db::TransferType::MIXED;
}

};
