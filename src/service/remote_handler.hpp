#pragma once
#include "remote_info.hpp"

#include <functional>
#include <future>

namespace srv
{

class RemoteHandler
{
public:
    RemoteHandler( RemoteInfoPtr info );
    void setEditListener( std::function<void( RemoteInfoPtr )> listener );
    void process();

private:
    static const int CHANNEL_RETRY_WAIT_TIME;
    static const int DUPLEX_MAX_FAILURES;
    static const int DUPLEX_WAIT_PING_TIME;
    static const int CONNECTED_PING_TIME;
    static const int CONNECT_TIMEOUT;
    static const int V2_CHANNEL_RETRY_WAIT_TIME;
    static const int V2_DUPLEX_TIMEOUT;

    RemoteInfoPtr m_info;
    int m_api;
    std::string m_ident;
    std::function<void( RemoteInfoPtr )> m_editHandler;

    bool secureLoopV1();
    bool secureLoopV2();

    bool checkDuplexConnection();
    void updateRemoteMachineInfo();
    void updateRemoteMachineAvatar();
    std::future<bool> waitForDuplex( int timeout );

    void setTimeout( grpc::ClientContext& context, int seconds );
    void setRemoteStatus( std::unique_lock<std::mutex>& lck, 
        RemoteStatus newStatus );
};

};
