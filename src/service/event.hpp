#pragma once

namespace srv
{

enum class EventType
{
    STOP_SERVICE,
    RESTART_SERVICE,
    REPEAT_MDNS_QUERY
};

struct Event
{
    EventType type;
    union
    {

    } eventData;
};

};
