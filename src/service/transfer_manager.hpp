#pragma once
#include "database_manager.hpp"
#include "event.hpp"
#include "file_crawler.hpp"
#include "observable_service.hpp"
#include "remote_manager.hpp"
#include "transfer_types.hpp"
#include "zlib_deflate.hpp"

#include <wx/file.h>
#include <wx/wx.h>

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

namespace srv
{

typedef std::shared_ptr<TransferOp> TransferOpPtr;

class RemoteManager;

class TransferManager
{
public:
    explicit TransferManager( ObservableService* service );
    ~TransferManager();

    void setOutputPath( const std::wstring& outputPath );
    std::wstring getOutputPath();

    void setRemoteManager( std::shared_ptr<RemoteManager> ptr );
    RemoteManager* getRemoteManager();

    void setDatabaseManager( std::shared_ptr<DatabaseManager> ptr );
    DatabaseManager* getDatabaseManager();
    
    void setCrawlerPtr( std::shared_ptr<FileCrawler> ptr );
    FileCrawler* getFileCrawler();

    void setCompressionLevel( int level );
    int getCompressionLevel();
    
    void setMustAllowIncoming( bool must );
    bool getMustAllowIncoming();

    void setMustAllowOverwrite( bool must );
    bool getMustAllowOverwrite();

    void setUnixPermissionMasks( int file, int executable, int directory );
    int getUnixFilePermissionMask();
    int getUnixExecutablePermissionMask();
    int getUnixDirectoryPermissionMask();

    void stop();

    TransferOpPtr registerTransfer( const std::string& remoteId, 
        TransferOp& transfer, bool firstTry );

    void replyAllowTransfer( const std::string& remoteId,
        int transferId, bool allow );
    void cancelTransferRequest( const std::string& remoteId,
        time_t timestamp );
    void resumeTransfer( const std::string& remoteId, int transferId );
    void pauseTransfer( const std::string& remoteId, int transferId );
    void stopTransfer( const std::string& remoteId, int transferId, bool error );
    void finishTransfer( const std::string& remoteId, int transferId );
    void requestStopTransfer( const std::string& remoteId, 
        int transferId, bool error );

    /* Blocking function, call on background thread only! */
    bool handleOutcomingTransfer( const std::string& remoteId, 
        int transferId, grpc::ServerWriter<FileChunk>* writer, bool compress );
    void failAll( const std::string& remoteId );

    int createOutcomingTransfer( const std::string& remoteId, 
        const std::vector<std::wstring>& rootPaths );
    void notifyCrawlerSucceeded( const CrawlerOutputData& data );
    void notifyCrawlerFailed( int requestId );

    std::mutex& getMutex();

    std::vector<TransferOpPtr> getTransfersForRemote( 
        const std::string& remoteId );
    TransferOpPub getOp( const std::string& remoteId, time_t timestamp );

private:
    class StartTransferReactor 
        : public grpc::experimental::ClientReadReactor<FileChunk>
    {
    public:
        void setInstance( std::shared_ptr<StartTransferReactor> selfPtr );
        void setRefs( std::shared_ptr<grpc::ClientContext> ref1,
            std::shared_ptr<OpInfo> ref2 );
        void setTransferPtr( TransferOpPtr transferPtr );
        void setRemoteId( const std::string& remoteId );
        void setManager( TransferManager* mgr );
        void setCanOverwrite( bool canOverwrite );

        void start();

        void OnDone( const grpc::Status& s ) override;
        void OnReadDone( bool ok ) override;
    private:
        std::shared_ptr<StartTransferReactor> m_selfPtr;

        // gRPC ref holders - they will be released after this reader object
        // deletes itself
        std::shared_ptr<grpc::ClientContext> m_clientCtx;
        std::shared_ptr<OpInfo> m_request;

        TransferOpPtr m_transfer;
        std::string m_remoteId;
        TransferManager* m_mgr;

        FileChunk m_chunk;
        ZlibDeflate m_compressor;
        bool m_useCompression;
        bool m_canOverwrite;
        
        std::string m_filePtrPath;
        wxFile m_filePtr;

        wxString getAbsolutePath( wxString relativePath );
        void updatePaths();
        void updateProgress( long long chunkBytes );
        void processData( const std::string& dataChunk );
        void failOp();
    };

    static const long long PROGRESS_FREQ_MILLIS;
    static const std::string DEFAULT_MIME_TYPE;
    static const std::string DIRECTORY_MIME_TYPE;

    std::atomic_bool m_running;
    std::shared_ptr<RemoteManager> m_remoteMgr;
    std::shared_ptr<DatabaseManager> m_dbMgr;
    std::shared_ptr<FileCrawler> m_crawler;
    std::map<int, TransferOpPtr> m_crawlJobs;

    ObservableService* m_srv;
    int m_lastId;
    std::mutex m_mtx;
    std::wstring m_outputPath;
    int m_compressionLevel;
    bool m_mustAllowIncoming;
    bool m_mustAllowOverwrite;

    int m_filePerms;
    int m_execPerms;
    int m_dirPerms;

    TransferOp m_empty;

    std::map<std::string, std::vector<TransferOpPtr>> m_transfers;

    void checkTransferDiskSpace( TransferOp& op );
    void checkTransferMustOverwrite( TransferOp& op );
    void setTransferTimestamp( TransferOp& op );
    void setUpPauseLock( TransferOp& op );
    void sendNotifications( const std::string& remoteId, TransferOp& op,
        bool actionRequired );

    TransferOpPtr getTransferInfo( const std::string& remoteId, int transferId );
    TransferOpPtr getTransferByTimestamp(
        const std::string& remoteId, time_t timestamp );

    void processStartTransfer( const std::string& remoteId, TransferOpPtr op );
    void processDeclineTransfer( const std::string& remoteId, TransferOpPtr op );

    OpInfo convertOpToOpInfo( const TransferOpPtr op, 
        bool compressionEnabled ) const;
    void sendStatusUpdateNotification( const std::string& remoteId, 
        const TransferOpPtr op );
    void sendFailureNotification( const std::string& remoteId, 
        const std::wstring& senderName );
    void sendSuccessNotification( const std::string& remoteId,
        const std::wstring& senderName );

    void doReplyAllowTransfer( const std::string& remoteId,
        int transferId, bool allow );
    void doFinishTransfer( const std::string& remoteId, int transferId );
    void doSendRequestAfterCrawling( TransferOpPtr op, 
        const CrawlerOutputData& data );
    bool failOp( TransferOpPtr op );

    db::TransferStatus getOpStatus( const TransferOpPtr op );
    db::TransferType getOpType( const TransferOpPtr op );
};

};
