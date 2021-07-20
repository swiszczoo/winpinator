#pragma once
#include "../zeroconf/mdns_types.hpp"

#include <condition_variable>
#include <memory>
#include <mutex>
#include <set>
#include <vector>

namespace srv
{

enum class RemoteStatus
{
    OFFLINE,
    REGISTRATION,
    INIT_CONNECTING,
    UNREACHABLE,
    AWAITING_DUPLEX,
    ONLINE
};

struct RemoteInfo
{
    RemoteStatus state;

    std::string id;

    // mDNS metadata
    zc::MdnsIpPair ips;
    int port;
    int authPort;
    int apiVersion;
    std::string os;
    std::string hostname;

    std::thread thread;
    std::mutex mutex;
    std::condition_variable stopVar;
    bool stopping;

    // Host should be visible after its public key has been successfully
    // decoded using key group
    bool visible;
};

typedef std::shared_ptr<RemoteInfo> RemoteInfoPtr;

};