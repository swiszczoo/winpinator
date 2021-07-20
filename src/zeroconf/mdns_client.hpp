#pragma once
#include "mdns.h"

#include "mdns_types.hpp"

#include <functional>
#include <map>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <iphlpapi.h>
#include <winsock2.h>
#define sleep( x ) Sleep( x * 1000 )
#else
#include <ifaddrs.h>
#include <netdb.h>
#endif

namespace zc
{

typedef std::function<void( const MdnsServiceData& )> AddListenerType;
typedef std::function<void( const std::string& )> RemoveListenerType;

class MdnsClient
{
public:
    MdnsClient( const std::string& serviceType );
    ~MdnsClient();

    // Warning! Don't modify the MdnsClient instance during execution of the
    // listener, otherwise a deadlock might occur!
    // 
    // Secondly, remember that listener callback runs on a background thread!
    void setOnAddServiceListener( AddListenerType listener );

    // Warning! Don't modify the MdnsClient instance during execution of the
    // listener, otherwise a deadlock might occur!
    // 
    // Secondly, remember that listener callback runs on a background thread!
    void setOnRemoveServiceListener( RemoveListenerType listener );

    void setIgnoredHostname( const std::string& ignored );
    std::string getIgnoredHostname();

    void startListening();
    void stopListening();
    bool isListening() const;

    // You may call this function from time to time to ensure that there are
    // no services left undiscovered due to the UDP packet loss.
    void repeatQuery();

private:
    std::string m_srvType;

    bool m_running;
    std::thread m_worker;

    std::mutex m_mtRunning;
    std::mutex m_mtListeners;
    std::recursive_mutex m_mtIgnored;

    AddListenerType m_addListener;
    RemoveListenerType m_removeListener;

    std::map<std::string, std::string> m_aRecords;
    std::map<std::string, std::string> m_aaaaRecords;

    std::map<std::string, MdnsServiceData> m_activeServices;

    std::queue<std::string> m_addedServices;
    std::queue<std::string> m_removedServices;
    std::string m_lastServiceName;
    int m_lastServiceTtl;

    std::string m_ignoredHost;

    int workerImpl();
    bool isValidService( const std::string& name );
    void processEvents();
    void unlockThread();

    // mDNS.c implementations
    sockaddr_in m_serviceAddressIpv4;
    sockaddr_in6 m_serviceAddressIpv6;
    int m_hasIpv4;
    int m_hasIpv6;

    int m_socketToKill;

    char m_addrbuffer[64];
    char m_entrybuffer[256];
    char m_namebuffer[256];
    char m_sendbuffer[1024];
    mdns_record_txt_t m_txtbuffer[128];

    mdns_string_t ipv4AddressToString( char* buffer, size_t capacity,
        const struct sockaddr_in* addr, size_t addrlen );
    mdns_string_t ipv6AddressToString( char* buffer, size_t capacity,
        const struct sockaddr_in6* addr, size_t addrlen );
    mdns_string_t ipAddressToString( char* buffer, size_t capacity, 
        const struct sockaddr* addr, size_t addrlen );

    int openClientSockets( int* sockets, int max_sockets, int port );
    int sendMdnsQuery( const char* service, int record );
    static int queryCallback( int sock, const struct sockaddr* from,
        size_t addrlen, mdns_entry_type_t entry, uint16_t query_id,
        uint16_t rtype, uint16_t rclass, uint32_t ttl, const void* data,
        size_t size, size_t name_offset, size_t name_length,
        size_t record_offset, size_t record_length, void* user_data );
};

};
