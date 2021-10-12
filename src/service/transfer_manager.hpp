#pragma once
#include "observable_service.hpp"
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
    std::atomic_bool m_running;

    ObservableService* m_srv;
    int m_lastId;
    std::mutex m_mtx;
    std::wstring m_outputPath;

    std::map<std::string, std::vector<TransferOp>> m_transfers;

    void checkTransferDiskSpace( TransferOp& op );
    void checkTransferMustOverwrite( TransferOp& op );
    void setTransferTimestamp( TransferOp& op );
};

};
