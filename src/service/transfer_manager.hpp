#pragma once
#include "observable_service.hpp"
#include "transfer_types.hpp"

#include <map>
#include <mutex>
#include <vector>

namespace srv
{

class TransferManager
{
public:
    explicit TransferManager( ObservableService* service );

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
    ObservableService* m_srv;
    int m_lastId;
    std::mutex m_mtx;

    std::map<std::string, std::vector<TransferOp>> m_transfers;
};

};
