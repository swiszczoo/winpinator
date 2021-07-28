#pragma once
#include "../proto-gen/warp.grpc.pb.h"

#include "remote_manager.hpp"

#include <grpcpp/server.h>

#include <cstdint>
#include <future>
#include <memory>

namespace srv
{

using grpc::Status;
using grpc::ServerContext;

class WarpServiceImpl : public Warp::Service
{
public:
    explicit WarpServiceImpl( std::shared_ptr<RemoteManager> mgr );

protected:
    Status Ping( grpc::ServerContext* context,  const LookupName* request, 
        VoidType* response ) override;
    Status CheckDuplexConnection( grpc::ServerContext* context, 
        const LookupName* request, HaveDuplex* response ) override;

private:
    std::shared_ptr<RemoteManager> m_manager;
};

class WarpServer
{
public:
    WarpServer();
    ~WarpServer();

    void setPort( uint16_t port );
    uint16_t getPort() const;

    void setPemPrivateKey( const std::string& pem );
    const std::string& getPemPrivateKey() const;

    void setPemCertificate( const std::string& pem );
    const std::string& getPemCertificate() const;

    void setRemoteManager( std::shared_ptr<RemoteManager> mgr );
    std::shared_ptr<RemoteManager> getRemoteManager() const;

    bool startServer();
    bool stopServer();

private:
    static const int DEFAULT_SERVICE_PORT;

    uint16_t m_port;
    std::string m_privKey;
    std::string m_pubKey;
    std::shared_ptr<RemoteManager> m_remoteMgr;
    std::unique_ptr<grpc::Server> m_server;
    std::thread m_thread;

    int threadMain( uint16_t port, std::string priv, std::string pub,
        std::shared_ptr<RemoteManager> mgr, std::promise<void>& startProm );
};

};
