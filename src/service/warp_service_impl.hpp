#pragma once
#include "../proto-gen/warp.grpc.pb.h"

#include "remote_manager.hpp"
#include "transfer_manager.hpp"

#include <grpcpp/server.h>

#include <cstdint>
#include <future>
#include <memory>
#include <string>

namespace srv
{

using grpc::Status;
using grpc::ServerContext;

class WarpServiceImpl : public Warp::Service
{
public:
    explicit WarpServiceImpl( std::shared_ptr<RemoteManager> remoteMgr, 
        std::shared_ptr<TransferManager> transferMgr,
        std::shared_ptr<std::string> avatar );

protected:
    Status Ping( grpc::ServerContext* context, const LookupName* request, 
        VoidType* response ) override;

    Status CheckDuplexConnection( grpc::ServerContext* context, 
        const LookupName* request, HaveDuplex* response ) override;
    Status WaitingForDuplex( grpc::ServerContext* context,
        const LookupName* request, HaveDuplex* response ) override;

    Status GetRemoteMachineInfo( grpc::ServerContext* context,
        const LookupName* request, RemoteMachineInfo* response ) override;
    Status GetRemoteMachineAvatar( grpc::ServerContext* context,
        const LookupName* request, 
        grpc::ServerWriter<RemoteMachineAvatar>* writer ) override;
    Status ProcessTransferOpRequest( grpc::ServerContext* context,
        const TransferOpRequest* request, VoidType* response ) override;

    Status StartTransfer( grpc::ServerContext* context,
        const OpInfo* request, grpc::ServerWriter<FileChunk>* writer ) override;

private:
    std::shared_ptr<RemoteManager> m_remoteMgr;
    std::shared_ptr<TransferManager> m_transferMgr;
    std::shared_ptr<std::string> m_avatar;
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

    void setAvatarBytes( const std::string& bytes );
    const std::string& getAvatarBytes() const;

    void setRemoteManager( std::shared_ptr<RemoteManager> mgr );
    std::shared_ptr<RemoteManager> getRemoteManager() const;

    void setTransferManager( std::shared_ptr<TransferManager> mgr );
    std::shared_ptr<TransferManager> getTransferManager() const;

    bool startServer();
    bool stopServer();

private:
    static const int DEFAULT_SERVICE_PORT;
    static const std::string EMPTY;

    uint16_t m_port;
    std::string m_privKey;
    std::string m_pubKey;
    std::shared_ptr<std::string> m_avatar;
    std::shared_ptr<RemoteManager> m_remoteMgr;
    std::shared_ptr<TransferManager> m_transferMgr;
    std::unique_ptr<grpc::Server> m_server;
    std::thread m_thread;

    int threadMain( uint16_t port, std::string priv, std::string pub,
        std::shared_ptr<RemoteManager> remoteMgr,
        std::shared_ptr<TransferManager> transferMgr,
        std::shared_ptr<std::string> avatar, 
        std::promise<bool>& startProm );
};

};
