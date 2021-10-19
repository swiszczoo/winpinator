#pragma once
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
    STOP_SERVICE,
    RESTART_SERVICE,
    REPEAT_MDNS_QUERY,
    HOST_ADDED,
    HOST_REMOVED,
    SHOW_TOAST_NOTIFICATION,
    ACCEPT_TRANSFER_CLICKED,
    DECLINE_TRANSFER_CLICKED
};

struct TransferData
{
    std::string remoteId;
    int transferId;
};

struct Event
{
    EventType type;
    struct
    {
        std::shared_ptr<zc::MdnsServiceData> addedData;
        std::shared_ptr<std::string> removedData;
        std::shared_ptr<ToastNotification> toastData;
        std::shared_ptr<TransferData> transferData;
    } eventData;
};

};
