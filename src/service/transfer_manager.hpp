#pragma once
#include "observable_service.hpp"
#include "remote_manager.hpp"
#include "transfer_types.hpp"

#include <wx/wx.h>

#include <atomic>
#include <map>
#include <mutex>
#include <vector>

namespace srv
{

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
        TransferOp& transfer );
    void replyAllowTransfer( const std::string& remoteId,
        int transferId, bool allow );
    void resumeTransfer( const std::string& remoteId, 
        int transferId );
    void pauseTransfer( const std::string& remoteId,
        int transferId );
    void stopTransfer( const std::string& remoteId,
        int transferId );
    void finishTransfer( const std::string& remoteId, int transferId );

    std::vector<TransferOp> getTransfersForRemote( const std::string& remoteId );

private:
    class StartTransferReactor 
        : public grpc::experimental::ClientReadReactor<FileChunk>
    {
    public:
        void setInstance( std::shared_ptr<StartTransferReactor> selfPtr );
        void setRefs( std::shared_ptr<grpc::ClientContext> ref1,
            std::shared_ptr<OpInfo> ref2 );

        void start();

        void OnDone( const grpc::Status& s ) override;
        void OnReadDone( bool ok ) override;
    private:
        std::shared_ptr<StartTransferReactor> m_selfPtr;

        // gRPC ref holders - they will be released after this reader object
        // deletes itself
        std::shared_ptr<grpc::ClientContext> m_clientCtx;
        std::shared_ptr<OpInfo> m_request;

        FileChunk m_chunk;
    };

    std::atomic_bool m_running;
    std::shared_ptr<RemoteManager> m_remoteMgr;

    ObservableService* m_srv;
    int m_lastId;
    std::mutex m_mtx;
    std::wstring m_outputPath;
    TransferOp m_empty;

    std::map<std::string, std::vector<TransferOp>> m_transfers;

    void checkTransferDiskSpace( TransferOp& op );
    void checkTransferMustOverwrite( TransferOp& op );
    void setTransferTimestamp( TransferOp& op );
    void sendNotifications( const std::string& remoteId, TransferOp& op );
    TransferOp& getTransferInfo( const std::string& remoteId, int transferId );

    void processStartOrResumeTransfer( const std::string& remoteId, 
        TransferOp& op );
};

};
