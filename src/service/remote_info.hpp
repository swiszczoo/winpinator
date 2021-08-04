#pragma once
#include "../zeroconf/mdns_types.hpp"

#include "../proto-gen/warp.grpc.pb.h"

#include <atomic>
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

    std::string shortName;
    std::wstring fullName;
    std::string avatarBuffer;

    // Synchronization stuff
    std::thread thread;
    std::mutex mutex;
    std::condition_variable stopVar;
    std::atomic_bool stopping;

    // Flags
    bool busy;
    bool hasZcPresence;

    // Host should be visible after its public key has been successfully
    // decoded using group key 
    bool visible;

    // gRPC stuff
    std::shared_ptr<Warp::Stub> stub;
};

typedef std::shared_ptr<RemoteInfo> RemoteInfoPtr;

};