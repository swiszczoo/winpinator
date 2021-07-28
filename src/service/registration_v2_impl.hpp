#pragma once
#include "../proto-gen/warp.grpc.pb.h"

#include <cstdint>
#include <future>

namespace srv
{

using grpc::Status;
using grpc::ServerContext;

class RegistrationV2Impl : public WarpRegistration::Service
{
    Status RequestCertificate( ServerContext* context, 
        const RegRequest* request, RegResponse* response ) override;
};

class RegistrationV2Server
{
public:
    RegistrationV2Server();
    ~RegistrationV2Server();

    void setPort( uint16_t port );
    uint16_t getPort() const;

    bool startServer();
    bool stopServer();

private:
    uint16_t m_port;
    std::unique_ptr<grpc::Server> m_server;
    std::thread m_thread;

    int threadMain( uint16_t port, std::promise<void>& startProm );
};

};