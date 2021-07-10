#pragma once

namespace srv
{

enum class EventType
{
    STOP_SERVICE,
    RESTART_SERVICE
};

struct Event
{
    EventType type;
};

};
