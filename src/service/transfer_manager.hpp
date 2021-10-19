#pragma once
#include "observable_service.hpp"
#include "remote_manager.hpp"
#include "transfer_types.hpp"
#include "zlib_deflate.hpp"

#include <wx/wx.h>

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

namespace srv
{

typedef std::shared_ptr<TransferOp> TransferOpPtr;

class TransferManager
{
public:
    explicit TransferManager( ObservableService* service );
    ~TransferManager();

    void setOutputPath( const std::wstring& outputPath );
    std::wstring getOutputPath();

    void setRemoteManager( std::shared_ptr<RemoteManager> ptr );
    RemoteManager* getRemoteManager();

    void stop();

    void registerTransfer( const std::string& remoteId, 
        TransferOp& transfer, bool firstTry );

    void replyAllowTransfer( const std::string& remoteId,
        int transferId, bool allow );
    void resumeTransfer( const std::string& remoteId, 
        int transferId );
    void pauseTransfer( const std::string& remoteId,
        int transferId );
    void stopTransfer( const std::string& remoteId,
        int transferId );
    void finishTransfer( const std::string& remoteId, int transferId );

    std::mutex& getMutex();

    std::vector<TransferOpPtr> getTransfersForRemote( 
        const std::string& remoteId );

private:
    class StartTransferReactor 
        : public grpc::experimental::ClientReadReactor<FileChunk>
    {
    public:
        void setInstance( std::shared_ptr<StartTransferReactor> selfPtr );
        void setRefs( std::shared_ptr<grpc::ClientContext> ref1,
            std::shared_ptr<OpInfo> ref2 );
        void setTransferPtr( TransferOpPtr transferPtr );
        void setManager( TransferManager* mgr );

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
        TransferManager* m_mgr;

        FileChunk m_chunk;
        ZlibDeflate m_compressor;
        bool m_useCompression;

        void updateProgress( long long chunkBytes );
    };

    static const long long PROGRESS_FREQ_MILLIS;

    std::atomic_bool m_running;
    std::shared_ptr<RemoteManager> m_remoteMgr;

    ObservableService* m_srv;
    int m_lastId;
    std::mutex m_mtx;
    std::wstring m_outputPath;
    TransferOp m_empty;

    std::map<std::string, std::vector<TransferOpPtr>> m_transfers;

    void checkTransferDiskSpace( TransferOp& op );
    void checkTransferMustOverwrite( TransferOp& op );
    void setTransferTimestamp( TransferOp& op );
    void setUpPauseLock( TransferOp& op );
    void sendNotifications( const std::string& remoteId, TransferOp& op );
    TransferOpPtr getTransferInfo( const std::string& remoteId, int transferId );

    void processStartOrResumeTransfer( const std::string& remoteId, 
        TransferOpPtr op );

    OpInfo convertOpToOpInfo( const TransferOpPtr op, 
        bool compressionEnabled ) const;
    void sendStatusUpdateNotification( const TransferOpPtr op );
};

};
