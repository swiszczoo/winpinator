#pragma once
#include "mdns.h"

#include <map>
#include <mutex>
#include <string>
#include <thread>

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <iphlpapi.h>
#define sleep( x ) Sleep( x * 1000 )
#else
#include <ifaddrs.h>
#include <netdb.h>
#endif


namespace zc
{

class MdnsService
{
public:
    explicit MdnsService( const std::string& serviceType );
    ~MdnsService();

    void setHostname( const std::string& hostname );
    std::string getHostname() const;

    void setPort( std::uint16_t port );
    std::uint16_t getPort() const;

    void registerService();
    void unregisterService();

    void setTxtRecord( const std::string& name, const std::string& value );
    void removeAllTxtRecords();
    size_t getTxtRecordsCount();

    bool isServiceRunning();

private:
    std::string m_srvType;
    std::string m_hostname;
    std::uint16_t m_port;

    bool m_running;
    std::thread m_worker;

    std::shared_ptr<std::mutex> m_mtRunning;

    std::map<std::string, std::string> m_txtRecords;

    int workerImpl();

    // mDNS.c implementations
    sockaddr_in m_serviceAddressIpv4;
    sockaddr_in6 m_serviceAddressIpv6;
    int m_hasIpv4;
    int m_hasIpv6;

    char m_addrbuffer[64];
    char m_entrybuffer[256];
    char m_namebuffer[256];
    char m_sendbuffer[1024];

    int openClientSockets( int* sockets, int maxSockets, int port );
    int openServiceSockets( int* sockets, int maxSockets );
    int serviceMdns( const char* hostname, 
        const char* serviceNname, int servicePort,
        std::shared_ptr<std::mutex> mutexRef );

    static int serviceCallback( int sock, const sockaddr* from,
        size_t addrlen, mdns_entry_type entry, uint16_t query_id,
        uint16_t rtype, uint16_t rclass, uint32_t ttl, const void* data,
        size_t size, size_t name_offset, size_t name_length,
        size_t record_offset, size_t record_length, void* user_data );
};

};
