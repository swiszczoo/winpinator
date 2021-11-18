#pragma once
#include "../settings_model.hpp"
#include "../zeroconf/mdns_types.hpp"
#include "notification.hpp"

#include <wintoast/wintoastlib.h>

#include <memory>

namespace zc
{
struct MdnsServiceData;
}

namespace srv
{

enum class EventType
{
    STOP_SERVICE = 1,
    RESTART_SERVICE,
    REPEAT_MDNS_QUERY,
    HOST_ADDED,
    HOST_REMOVED,
    SHOW_TOAST_NOTIFICATION,
    ACCEPT_TRANSFER_CLICKED,
    DECLINE_TRANSFER_CLICKED,
    STOP_TRANSFER,
    REQUEST_OUTCOMING_TRANSFER,
    OUTCOMING_CRAWLER_SUCCEEDED,
    OUTCOMING_CRAWLER_FAILED
};

struct TransferData
{
    std::string remoteId;
    int transferId;
};

struct OutcomingTransferData
{
    std::string remoteId;
    std::vector<std::wstring> droppedPaths;
};

struct CrawlerOutputData
{
    int jobId;
    std::wstring rootDir;
    std::shared_ptr<std::vector<std::wstring>> paths;
    std::vector<std::string> topDirBasenamesUtf8;
    long long totalSize;
    int folderCount;
    int fileCount;
};

struct Event
{
    EventType type;
    struct
    {
        std::shared_ptr<SettingsModel> restartData;
        std::shared_ptr<zc::MdnsServiceData> addedData;
        std::shared_ptr<std::string> removedData;
        std::shared_ptr<ToastNotification> toastData;
        std::shared_ptr<TransferData> transferData;
        std::shared_ptr<OutcomingTransferData> outcomingTransferData;
        int crawlerFailJobId;
        std::shared_ptr<CrawlerOutputData> crawlerOutputData;
    } eventData;
};

};
