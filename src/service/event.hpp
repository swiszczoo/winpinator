#pragma once
#include "../zeroconf/mdns_types.hpp"

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
    HOST_REMOVED
};

struct Event
{
    EventType type;
    struct
    {
        std::shared_ptr<zc::MdnsServiceData> addedData;
        std::shared_ptr<std::string> removedData;
    } eventData;
};

};
