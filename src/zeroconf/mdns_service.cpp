#include "mdns_service.hpp"

#include "../thread_name.hpp"

#include <memory>
#include <vector>

#include <stdlib.h>

namespace zc
{

struct service_t
{
    mdns_string_t service;
    mdns_string_t hostname;
    mdns_string_t service_instance;
    mdns_string_t hostname_qualified;
    struct sockaddr_in address_ipv4;
    struct sockaddr_in6 address_ipv6;
    int port;
    mdns_record_t record_ptr;
    mdns_record_t record_srv;
    mdns_record_t record_a;
    mdns_record_t record_aaaa;
    std::vector<mdns_record_t> txt_records;

    MdnsService* obj;
};

MdnsService::MdnsService( const std::string& serviceType )
    : m_srvType( serviceType )
    , m_hostname( "localhost" )
    , m_port( 80 )
    , m_running( false )
    , m_mtRunning( nullptr )
    , m_serviceAddressIpv4( { 0 } )
    , m_serviceAddressIpv6( { 0 } )
    , m_hasIpv4( false )
    , m_hasIpv6( false )
    , m_socketToKill( 0 )
    , m_addrbuffer( "" )
    , m_entrybuffer( "" )
    , m_namebuffer( "" )
    , m_sendbuffer( "" )
{
    // Create a shared mutex

    m_mtRunning = std::make_shared<std::recursive_mutex>();
}

MdnsService::~MdnsService()
{
    std::unique_lock<std::recursive_mutex> lock( *m_mtRunning );
    if ( m_running )
    {
        unregisterService();
    }

    unlockThread();

    lock.unlock();

    if ( m_worker.joinable() )
    {
        m_worker.join();
    }
}

void MdnsService::setHostname( const std::string& hostname )
{
    if ( m_running )
    {
        return;
    }

    m_hostname = hostname;
}

std::string MdnsService::getHostname() const
{
    return m_hostname;
}

void MdnsService::setPort( std::uint16_t port )
{
    if ( m_running )
    {
        return;
    }

    m_port = port;
}

std::uint16_t MdnsService::getPort() const
{
    return m_port;
}

void MdnsService::setTxtRecord( const std::string& name,
    const std::string& value )
{
    if ( m_running )
    {
        return;
    }

    m_txtRecords[name] = value;
}

size_t MdnsService::getTxtRecordsCount()
{
    return m_txtRecords.size();
}

void MdnsService::removeAllTxtRecords()
{
    if ( m_running )
    {
        return;
    }

    m_txtRecords.clear();
}

std::future<MdnsIpPair> MdnsService::registerService()
{
    if ( m_running )
    {
        std::promise<MdnsIpPair> temp;

        MdnsIpPair invalid;
        invalid.valid = false;
        invalid.ipv4 = "---";
        invalid.ipv6 = "---";

        temp.set_value( invalid );

        return temp.get_future();
    }

    if ( m_worker.joinable() )
    {
        unlockThread();
        m_worker.join();
    }

    m_running = true;

    auto promise = std::make_shared<std::promise<MdnsIpPair>>();

    m_worker = std::thread( std::bind( &MdnsService::workerImpl,
        this, promise ) );

    return promise->get_future();
}

void MdnsService::unregisterService()
{
    std::lock_guard<std::recursive_mutex> lock( *m_mtRunning );

    if ( !m_running )
    {
        return;
    }

    m_running = false;

    // Multicast a mDNS datagram with TTL set to 0, to announce
    // that our service is no longer active

    int sockets[32];
    int numSockets = openServiceSockets( sockets,
        sizeof( sockets ) / sizeof( sockets[0] ) );

    // printf( "Opened %d socket%s for mDNS service\n",
    //    numSockets, numSockets ? "s" : "" );

    size_t serviceNameLength = strlen( m_srvType.c_str() );

    char* serviceNameBuffer = (char*)malloc( serviceNameLength + 2 );
    memcpy( serviceNameBuffer, m_srvType.c_str(), serviceNameLength );

    if ( serviceNameBuffer[serviceNameLength - 1] != '.' )
        serviceNameBuffer[serviceNameLength++] = '.';

    serviceNameBuffer[serviceNameLength] = '\0';
    char* serviceName = serviceNameBuffer;

    size_t hostnameLength = strlen( m_hostname.c_str() );

    char* hostnameBuffer = (char*)malloc( hostnameLength + 1 );
    memcpy( hostnameBuffer, m_hostname.c_str(), hostnameLength );
    hostnameBuffer[hostnameLength] = '\0';
    char* hostname = hostnameBuffer;

    size_t capacity = 2048;
    void* buffer = malloc( capacity );

    mdns_string_t serviceString = { serviceName, strlen( serviceName ) };
    mdns_string_t hostnameString = { hostname, strlen( hostname ) };

    // Build the service instance "<hostname>.<_service-name>._tcp.local." string
    char serviceInstanceBuffer[256] = { 0 };
    snprintf( serviceInstanceBuffer, sizeof( serviceInstanceBuffer ) - 1,
        "%.*s.%.*s", MDNS_STRING_FORMAT( hostnameString ),
        MDNS_STRING_FORMAT( serviceString ) );

    mdns_string_t serviceInstanceString = {
        serviceInstanceBuffer, strlen( serviceInstanceBuffer )
    };

    // Build the "<hostname>.local." string
    char qualifiedHostnameBuffer[256] = { 0 };
    snprintf( qualifiedHostnameBuffer,
        sizeof( qualifiedHostnameBuffer ) - 1, "%.*s.local.",
        MDNS_STRING_FORMAT( hostnameString ) );

    mdns_string_t hostnameQualifiedString = {
        qualifiedHostnameBuffer, strlen( qualifiedHostnameBuffer )
    };

    service_t service = { 0 };
    service.obj = this;
    service.service = serviceString;
    service.hostname = hostnameString;
    service.service_instance = serviceInstanceString;
    service.hostname_qualified = hostnameQualifiedString;
    service.address_ipv4 = m_serviceAddressIpv4;
    service.address_ipv6 = m_serviceAddressIpv6;
    service.port = m_port;

    // Setup our mDNS records

    // PTR record reverse mapping "<_service-name>._tcp.local." to
    // "<hostname>.<_service-name>._tcp.local."
    service.record_ptr.name = service.service;
    service.record_ptr.type = MDNS_RECORDTYPE_PTR;
    service.record_ptr.data.ptr.name = service.service_instance;

    // SRV record mapping "<hostname>.<_service-name>._tcp.local." to
    // "<hostname>.local." with port. Set weight & priority to 0.
    service.record_srv.name = service.service_instance;
    service.record_srv.type = MDNS_RECORDTYPE_SRV;
    service.record_srv.data.srv.name = service.hostname_qualified;
    service.record_srv.data.srv.port = service.port;
    service.record_srv.data.srv.priority = 0;
    service.record_srv.data.srv.weight = 0;

    // A/AAAA records mapping "<hostname>.local." to IPv4/IPv6 addresses
    service.record_a.name = service.hostname_qualified;
    service.record_a.type = MDNS_RECORDTYPE_A;
    service.record_a.data.a.addr = service.address_ipv4;

    service.record_aaaa.name = service.hostname_qualified;
    service.record_aaaa.type = MDNS_RECORDTYPE_AAAA;
    service.record_aaaa.data.aaaa.addr = service.address_ipv6;

    // Setup TXT records
    std::vector<std::unique_ptr<char[]>> buffers;

    for ( auto pair : m_txtRecords )
    {
        mdns_record_t tmpRecord;
        tmpRecord.name = service.service_instance;
        tmpRecord.type = MDNS_RECORDTYPE_TXT;

        auto keyBuffer = std::make_unique<char[]>( pair.first.length() + 1 );
        auto valBuffer = std::make_unique<char[]>( pair.second.length() + 1 );

        strcpy_s( keyBuffer.get(),
            pair.first.length() + 1, pair.first.c_str() );
        strcpy_s( valBuffer.get(),
            pair.second.length() + 1, pair.second.c_str() );

        tmpRecord.data.txt.key.str = keyBuffer.get();
        tmpRecord.data.txt.key.length = pair.first.length();

        tmpRecord.data.txt.value.str = valBuffer.get();
        tmpRecord.data.txt.value.length = pair.second.length();

        service.txt_records.push_back( tmpRecord );

        // Transfer ownership of character buffers to vector
        // to automatically free memory after leaving the scope

        buffers.push_back( std::move( keyBuffer ) );
        buffers.push_back( std::move( valBuffer ) );
    }

    // Send an announcement on startup of service
    {
        int additionalTotal = 3 + m_txtRecords.size();
        mdns_record_t* additional = new mdns_record_t[additionalTotal] { 0 };

        size_t additionalCount = 0;
        additional[additionalCount++] = service.record_srv;
        if ( service.address_ipv4.sin_family == AF_INET )
            additional[additionalCount++] = service.record_a;
        if ( service.address_ipv6.sin6_family == AF_INET6 )
            additional[additionalCount++] = service.record_aaaa;

        for ( auto& txt : service.txt_records )
        {
            additional[additionalCount++] = txt;
        }

        for ( int isock = 0; isock < numSockets; ++isock )
            mdns_announce_multicast( sockets[isock], buffer, capacity,
                service.record_ptr, 0, 0, additional, additionalCount,
                /* TTL is equal to 0 -> */ 0 );

        delete[] additional;
    }

    free( buffer );
    free( serviceNameBuffer );
    free( hostnameBuffer );

    for ( int isock = 0; isock < numSockets; ++isock )
        mdns_socket_close( sockets[isock] );
}

bool MdnsService::isServiceRunning()
{
    return m_running;
}

int MdnsService::workerImpl( std::shared_ptr<std::promise<MdnsIpPair>> promise )
{
    setThreadName( "mDNS service worker" );

    auto hostname = std::make_unique<char[]>( m_hostname.size() + 1 );
    auto servName = std::make_unique<char[]>( m_srvType.size() + 1 );

    strcpy_s( hostname.get(), m_hostname.size() + 1, m_hostname.c_str() );
    strcpy_s( servName.get(), m_srvType.size() + 1, m_srvType.c_str() );

    serviceMdns( hostname.get(), servName.get(), m_port, m_mtRunning,
        std::move( promise ) );

    return EXIT_SUCCESS;
}

void MdnsService::unlockThread()
{
    // HACK: Send an empty datagram to ensure select does not wait forever
    // (we're sure that m_running equals false at this moment)
#ifdef _MSC_VER
    //OutputDebugString( L"Unlocking thread...\n" );
#endif

    int sock = (int)socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );

    if ( sock == 0 )
    {
        return;
    }

    char buf = '\0';

    // Create destination address

    sockaddr_storage addr_storage;
    sockaddr_in addr;
    sockaddr* saddr = (struct sockaddr*)&addr_storage;
    socklen_t saddrlen = sizeof( struct sockaddr_storage );

    memset( &addr, 0, sizeof( addr ) );
    addr.sin_family = AF_INET;
#ifdef __APPLE__
    addr.sin_len = sizeof( addr );
#endif
    addr.sin_addr.s_addr = htonl( ( ( (uint32_t)127U ) << 24U ) | ( (uint32_t)1U ) );
    addr.sin_port = htons( (unsigned short)MDNS_PORT );
    saddr = (struct sockaddr*)&addr;
    saddrlen = sizeof( addr );

    mdns_unicast_send( sock, saddr, saddrlen, &buf, sizeof( buf ) );
#ifdef _MSC_VER
    //if ( result )
    //    OutputDebugString( L"Unlock FAILED!\n" );
    //else
    //    OutputDebugString( L"Unlock SUCCEEDED!\n" );
#endif

    mdns_socket_close( sock );

    // The method above doesn't always work
    // so...
    // Shamelessly kill one of the sockets select is listening on

    mdns_socket_close( m_socketToKill );
}

int MdnsService::openClientSockets( int* sockets, int maxSockets, int port )
{
    // When sending, each socket can only send to one network interface
    // Thus we need to open one socket for each interface and address family
    int numSockets = 0;

#ifdef _WIN32

    IP_ADAPTER_ADDRESSES* adapterAddress = 0;
    ULONG addressSize = 8000;
    unsigned int ret;
    unsigned int num_retries = 4;
    do
    {
        adapterAddress = (IP_ADAPTER_ADDRESSES*)malloc( addressSize );
        ret = GetAdaptersAddresses( AF_UNSPEC, GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_ANYCAST, 0,
            adapterAddress, &addressSize );
        if ( ret == ERROR_BUFFER_OVERFLOW )
        {
            free( adapterAddress );
            adapterAddress = 0;
        }
        else
        {
            break;
        }
    } while ( num_retries-- > 0 );

    if ( !adapterAddress || ( ret != NO_ERROR ) )
    {
        free( adapterAddress );
        printf( "Failed to get network adapter addresses\n" );
        return numSockets;
    }

    int firstIpv4 = 1;
    int firstIpv6 = 1;
    for ( PIP_ADAPTER_ADDRESSES adapter = adapterAddress; adapter; adapter = adapter->Next )
    {
        if ( adapter->TunnelType == TUNNEL_TYPE_TEREDO )
            continue;
        if ( adapter->OperStatus != IfOperStatusUp )
            continue;

        for ( IP_ADAPTER_UNICAST_ADDRESS* unicast = adapter->FirstUnicastAddress; unicast;
              unicast = unicast->Next )
        {
            if ( unicast->Address.lpSockaddr->sa_family == AF_INET )
            {
                struct sockaddr_in* saddr = (struct sockaddr_in*)unicast->Address.lpSockaddr;
                if ( ( saddr->sin_addr.S_un.S_un_b.s_b1 != 127 ) || ( saddr->sin_addr.S_un.S_un_b.s_b2 != 0 ) || ( saddr->sin_addr.S_un.S_un_b.s_b3 != 0 ) || ( saddr->sin_addr.S_un.S_un_b.s_b4 != 1 ) )
                {
                    int logAddr = 0;
                    if ( firstIpv4 )
                    {
                        m_serviceAddressIpv4 = *saddr;
                        firstIpv4 = 0;
                        logAddr = 1;
                    }
                    m_hasIpv4 = 1;
                    if ( numSockets < maxSockets )
                    {
                        saddr->sin_port = htons( (unsigned short)port );
                        int sock = mdns_socket_open_ipv4( saddr );
                        if ( sock >= 0 )
                        {
                            sockets[numSockets++] = sock;
                            logAddr = 1;
                        }
                        else
                        {
                            logAddr = 0;
                        }
                    }
                    /*
                    if ( log_addr )
                    {
                        char buffer[128];
                        mdns_string_t addr = ipv4_address_to_string( buffer, sizeof( buffer ), saddr,
                            sizeof( struct sockaddr_in ) );
                        printf( "Local IPv4 address: %.*s\n", MDNS_STRING_FORMAT( addr ) );
                    }
                    */
                }
            }
            else if ( unicast->Address.lpSockaddr->sa_family == AF_INET6 )
            {
                struct sockaddr_in6* saddr = (struct sockaddr_in6*)unicast->Address.lpSockaddr;
                static const unsigned char localhost[] = { 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 1 };
                static const unsigned char localhost_mapped[] = { 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0xff, 0xff, 0x7f, 0, 0, 1 };
                if ( ( unicast->DadState == NldsPreferred ) && memcmp( saddr->sin6_addr.s6_addr, localhost, 16 ) && memcmp( saddr->sin6_addr.s6_addr, localhost_mapped, 16 ) )
                {
                    int logAddr = 0;
                    if ( firstIpv6 )
                    {
                        m_serviceAddressIpv6 = *saddr;
                        firstIpv6 = 0;
                        logAddr = 1;
                    }
                    m_hasIpv6 = 1;
                    if ( numSockets < maxSockets )
                    {
                        saddr->sin6_port = htons( (unsigned short)port );
                        int sock = mdns_socket_open_ipv6( saddr );
                        if ( sock >= 0 )
                        {
                            sockets[numSockets++] = sock;
                            logAddr = 1;
                        }
                        else
                        {
                            logAddr = 0;
                        }
                    }
                }
            }
        }
    }

    free( adapterAddress );

#else

    struct ifaddrs* ifaddr = 0;
    struct ifaddrs* ifa = 0;

    if ( getifaddrs( &ifaddr ) < 0 )
        printf( "Unable to get interface addresses\n" );

    int first_ipv4 = 1;
    int first_ipv6 = 1;
    for ( ifa = ifaddr; ifa; ifa = ifa->ifa_next )
    {
        if ( !ifa->ifa_addr )
            continue;

        if ( ifa->ifa_addr->sa_family == AF_INET )
        {
            struct sockaddr_in* saddr = (struct sockaddr_in*)ifa->ifa_addr;
            if ( saddr->sin_addr.s_addr != htonl( INADDR_LOOPBACK ) )
            {
                int log_addr = 0;
                if ( first_ipv4 )
                {
                    service_address_ipv4 = *saddr;
                    first_ipv4 = 0;
                    log_addr = 1;
                }
                has_ipv4 = 1;
                if ( num_sockets < max_sockets )
                {
                    saddr->sin_port = htons( port );
                    int sock = mdns_socket_open_ipv4( saddr );
                    if ( sock >= 0 )
                    {
                        sockets[num_sockets++] = sock;
                        log_addr = 1;
                    }
                    else
                    {
                        log_addr = 0;
                    }
                }
                if ( log_addr )
                {
                    char buffer[128];
                    mdns_string_t addr = ipv4_address_to_string( buffer, sizeof( buffer ), saddr,
                        sizeof( struct sockaddr_in ) );
                    printf( "Local IPv4 address: %.*s\n", MDNS_STRING_FORMAT( addr ) );
                }
            }
        }
        else if ( ifa->ifa_addr->sa_family == AF_INET6 )
        {
            struct sockaddr_in6* saddr = (struct sockaddr_in6*)ifa->ifa_addr;
            static const unsigned char localhost[] = { 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 1 };
            static const unsigned char localhost_mapped[] = { 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0xff, 0xff, 0x7f, 0, 0, 1 };
            if ( memcmp( saddr->sin6_addr.s6_addr, localhost, 16 ) && memcmp( saddr->sin6_addr.s6_addr, localhost_mapped, 16 ) )
            {
                int log_addr = 0;
                if ( first_ipv6 )
                {
                    service_address_ipv6 = *saddr;
                    first_ipv6 = 0;
                    log_addr = 1;
                }
                has_ipv6 = 1;
                if ( num_sockets < max_sockets )
                {
                    saddr->sin6_port = htons( port );
                    int sock = mdns_socket_open_ipv6( saddr );
                    if ( sock >= 0 )
                    {
                        sockets[num_sockets++] = sock;
                        log_addr = 1;
                    }
                    else
                    {
                        log_addr = 0;
                    }
                }
                if ( log_addr )
                {
                    char buffer[128];
                    mdns_string_t addr = ipv6_address_to_string( buffer, sizeof( buffer ), saddr,
                        sizeof( struct sockaddr_in6 ) );
                    printf( "Local IPv6 address: %.*s\n", MDNS_STRING_FORMAT( addr ) );
                }
            }
        }
    }

    freeifaddrs( ifaddr );

#endif

    return numSockets;
}

int MdnsService::openServiceSockets( int* sockets, int maxSockets )
{
    // When recieving, each socket can recieve data from all network interfaces
    // Thus we only need to open one socket for each address family
    int numSockets = 0;

    // Call the client socket function to enumerate and get local addresses,
    // but not open the actual sockets
    openClientSockets( 0, 0, 0 );

    if ( numSockets < maxSockets )
    {
        struct sockaddr_in sockAddr;
        memset( &sockAddr, 0, sizeof( struct sockaddr_in ) );
        sockAddr.sin_family = AF_INET;
#ifdef _WIN32
        sockAddr.sin_addr = in4addr_any;
#else
        sock_addr.sin_addr.s_addr = INADDR_ANY;
#endif
        sockAddr.sin_port = htons( MDNS_PORT );
#ifdef __APPLE__
        sock_addr.sin_len = sizeof( struct sockaddr_in );
#endif
        int sock = mdns_socket_open_ipv4( &sockAddr );
        if ( sock >= 0 )
            sockets[numSockets++] = sock;
    }

    if ( numSockets < maxSockets )
    {
        struct sockaddr_in6 sockAddr;
        memset( &sockAddr, 0, sizeof( struct sockaddr_in6 ) );
        sockAddr.sin6_family = AF_INET6;
        sockAddr.sin6_addr = in6addr_any;
        sockAddr.sin6_port = htons( MDNS_PORT );
#ifdef __APPLE__
        sock_addr.sin6_len = sizeof( struct sockaddr_in6 );
#endif
        int sock = mdns_socket_open_ipv6( &sockAddr );
        if ( sock >= 0 )
            sockets[numSockets++] = sock;
    }

    return numSockets;
}

// Provide a mDNS service, answering incoming DNS-SD and mDNS queries
int MdnsService::serviceMdns( const char* hostname,
    const char* serviceName, int servicePort,
    std::shared_ptr<std::recursive_mutex> mutexRef,
    std::shared_ptr<std::promise<MdnsIpPair>> promise )
{
    // We hold a mutex reference here to prevent it from being destoyed
    // along with class instance and detaching this thread, in case
    // mutex is busy or will ever be used in the future
    (void)mutexRef;

    const uint32_t ttl = 3600; // seconds
    const uint32_t ucttl = 600; // seconds

    int sockets[32];
    int numSockets = openServiceSockets( sockets,
        sizeof( sockets ) / sizeof( sockets[0] ) );

    if ( numSockets <= 0 )
    {
        printf( "Failed to open any client sockets\n" );
        return -1;
    }

    // printf( "Opened %d socket%s for mDNS service\n", numSockets, numSockets ? "s" : "" );

    size_t serviceNameLength = strlen( serviceName );
    if ( !serviceNameLength )
    {
        printf( "Invalid service name\n" );
        return -1;
    }

    char* serviceNameBuffer = (char*)malloc( serviceNameLength + 2 );
    memcpy( serviceNameBuffer, serviceName, serviceNameLength );

    if ( serviceNameBuffer[serviceNameLength - 1] != '.' )
        serviceNameBuffer[serviceNameLength++] = '.';

    serviceNameBuffer[serviceNameLength] = 0;
    serviceName = serviceNameBuffer;

    // printf( "Service mDNS: %s:%d\n", serviceName, servicePort );
    // printf( "Hostname: %s\n", hostname );

    size_t capacity = 2048;
    void* buffer = malloc( capacity );

    mdns_string_t serviceString = { serviceName, strlen( serviceName ) };
    mdns_string_t hostnameString = { hostname, strlen( hostname ) };

    // Build the service instance "<hostname>.<_service-name>._tcp.local." string
    char serviceInstanceBuffer[256] = { 0 };
    snprintf( serviceInstanceBuffer, sizeof( serviceInstanceBuffer ) - 1,
        "%.*s.%.*s", MDNS_STRING_FORMAT( hostnameString ),
        MDNS_STRING_FORMAT( serviceString ) );

    mdns_string_t serviceInstanceString = {
        serviceInstanceBuffer, strlen( serviceInstanceBuffer )
    };

    // Build the "<hostname>.local." string
    char qualifiedHostnameBuffer[256] = { 0 };
    snprintf( qualifiedHostnameBuffer,
        sizeof( qualifiedHostnameBuffer ) - 1, "%.*s.local.",
        MDNS_STRING_FORMAT( hostnameString ) );

    mdns_string_t hostnameQualifiedString = {
        qualifiedHostnameBuffer, strlen( qualifiedHostnameBuffer )
    };

    service_t service = { 0 };
    service.obj = this;
    service.service = serviceString;
    service.hostname = hostnameString;
    service.service_instance = serviceInstanceString;
    service.hostname_qualified = hostnameQualifiedString;
    service.address_ipv4 = m_serviceAddressIpv4;
    service.address_ipv6 = m_serviceAddressIpv6;
    service.port = servicePort;

    // Setup our mDNS records

    // PTR record reverse mapping "<_service-name>._tcp.local." to
    // "<hostname>.<_service-name>._tcp.local."
    service.record_ptr.name = service.service;
    service.record_ptr.type = MDNS_RECORDTYPE_PTR;
    service.record_ptr.data.ptr.name = service.service_instance;

    // SRV record mapping "<hostname>.<_service-name>._tcp.local." to
    // "<hostname>.local." with port. Set weight & priority to 0.
    service.record_srv.name = service.service_instance;
    service.record_srv.type = MDNS_RECORDTYPE_SRV;
    service.record_srv.data.srv.name = service.hostname_qualified;
    service.record_srv.data.srv.port = service.port;
    service.record_srv.data.srv.priority = 0;
    service.record_srv.data.srv.weight = 0;

    // A/AAAA records mapping "<hostname>.local." to IPv4/IPv6 addresses
    service.record_a.name = service.hostname_qualified;
    service.record_a.type = MDNS_RECORDTYPE_A;
    service.record_a.data.a.addr = service.address_ipv4;

    service.record_aaaa.name = service.hostname_qualified;
    service.record_aaaa.type = MDNS_RECORDTYPE_AAAA;
    service.record_aaaa.data.aaaa.addr = service.address_ipv6;

    // Fulfill the promise with our IP addresses
    char ipv4Buf[128];
    std::string ipv6;

    sprintf_s( ipv4Buf, "%u.%u.%u.%u",
        service.address_ipv4.sin_addr.S_un.S_un_b.s_b1,
        service.address_ipv4.sin_addr.S_un.S_un_b.s_b2,
        service.address_ipv4.sin_addr.S_un.S_un_b.s_b3,
        service.address_ipv4.sin_addr.S_un.S_un_b.s_b4 );

    // Determine the longest series of zeroes in the IPv6 address
    int longestZeroesLength = 0;
    int longestZeroesStart = 0;
    int currentZeroesStart = 0;
    int currentZeroesLength = 0;

    for ( size_t i = 0; i < 8; i++ )
    {
        WORD hextet = service.address_ipv6.sin6_addr.u.Word[i];

        if ( hextet == 0 ) // NOTE: We may ignore reversed endianness here
        {
            currentZeroesLength++;
        }
        else
        {
            if ( currentZeroesLength > longestZeroesLength )
            {
                longestZeroesLength = currentZeroesLength;
                longestZeroesStart = currentZeroesStart;
            }

            currentZeroesStart = i + 1;
            currentZeroesLength = 0;
        }
    }

    if ( currentZeroesLength > longestZeroesLength )
    {
        longestZeroesLength = currentZeroesLength;
        longestZeroesStart = currentZeroesStart;
    }

    // Construct IPv6 address string
    for ( size_t i = 0; i < 8; i++ )
    {
        if ( i == longestZeroesStart && longestZeroesLength > 0 )
        {
            ipv6 += "::";
            continue;
        }

        if ( i < longestZeroesStart 
            || i >= longestZeroesStart + longestZeroesLength )
        {
            char hextetBuf[5];
            WORD hextet = service.address_ipv6.sin6_addr.u.Word[i];

            // We assume that our CPU is little-endian, so we swap the byte order
            WORD leHextet = ( ( hextet & 0xFF00 ) >> 8 ) 
                | ( ( hextet & 0xff ) << 8 );

            sprintf_s( hextetBuf, "%x", leHextet );

            if ( !ipv6.empty() && ipv6[ipv6.length() - 1] != ':' )
            {
                ipv6 += ':';
            }
            ipv6 += hextetBuf;
        }
    }

    MdnsIpPair results;
    results.valid = true;
    results.ipv4 = std::string( ipv4Buf );
    results.ipv6 = ipv6;
    promise->set_value( results );

    // Setup TXT records
    std::vector<std::unique_ptr<char[]>> buffers;

    for ( auto pair : m_txtRecords )
    {
        mdns_record_t tmpRecord;
        tmpRecord.name = service.service_instance;
        tmpRecord.type = MDNS_RECORDTYPE_TXT;

        auto keyBuffer = std::make_unique<char[]>( pair.first.length() + 1 );
        auto valBuffer = std::make_unique<char[]>( pair.second.length() + 1 );

        strcpy_s( keyBuffer.get(),
            pair.first.length() + 1, pair.first.c_str() );
        strcpy_s( valBuffer.get(),
            pair.second.length() + 1, pair.second.c_str() );

        tmpRecord.data.txt.key.str = keyBuffer.get();
        tmpRecord.data.txt.key.length = pair.first.length();

        tmpRecord.data.txt.value.str = valBuffer.get();
        tmpRecord.data.txt.value.length = pair.second.length();

        service.txt_records.push_back( tmpRecord );

        // Transfer ownership of character buffers to vector
        // to automatically free memory after leaving the scope

        buffers.push_back( std::move( keyBuffer ) );
        buffers.push_back( std::move( valBuffer ) );
    }

    // Send an announcement on startup of service
    {
        int additionalTotal = 3 + m_txtRecords.size();
        mdns_record_t* additional = new mdns_record_t[additionalTotal] { 0 };

        size_t additionalCount = 0;
        additional[additionalCount++] = service.record_srv;
        if ( service.address_ipv4.sin_family == AF_INET )
            additional[additionalCount++] = service.record_a;
        if ( service.address_ipv6.sin6_family == AF_INET6 )
            additional[additionalCount++] = service.record_aaaa;

        for ( auto& txt : service.txt_records )
        {
            additional[additionalCount++] = txt;
        }

        for ( int isock = 0; isock < numSockets; ++isock )
            mdns_announce_multicast( sockets[isock], buffer, capacity,
                service.record_ptr, 0, 0, additional, additionalCount, ttl );

        delete[] additional;
    }

    // This is a crude implementation that checks for incoming queries
    while ( true )
    {
        int nfds = 0;
        fd_set readfs;
        FD_ZERO( &readfs );

        for ( int isock = 0; isock < numSockets; ++isock )
        {
            if ( sockets[isock] >= nfds )
                nfds = sockets[isock] + 1;
            FD_SET( sockets[isock], &readfs );
        }

        m_socketToKill = sockets[0];

        if ( select( nfds, &readfs, 0, 0, 0 ) >= 0 )
        {
#ifdef _MSC_VER
            //OutputDebugString( L"Select passed\n" );
#endif
            std::lock_guard<std::recursive_mutex> lock( *mutexRef );

            if ( mutexRef.use_count() < 2 || !m_running )
            {
                // Exit the thread if we are asked to
                // unregister the service

                break;
            }

            for ( int isock = 0; isock < numSockets; ++isock )
            {
                if ( FD_ISSET( sockets[isock], &readfs ) )
                {
                    mdns_socket_listen( sockets[isock], buffer, capacity,
                        MdnsService::serviceCallback, &service );
                }
                FD_SET( sockets[isock], &readfs );
            }
        }
        else
        {
            break;
        }
    }

    free( buffer );
    free( serviceNameBuffer );

    for ( int isock = 0; isock < numSockets; ++isock )
        mdns_socket_close( sockets[isock] );
    //printf( "Closed socket%s\n", numSockets ? "s" : "" );

    return 0;
}

int MdnsService::serviceCallback( int sock, const sockaddr* from,
    size_t addrlen, mdns_entry_type entry, uint16_t query_id, uint16_t rtype,
    uint16_t rclass, uint32_t ttl, const void* data, size_t size,
    size_t name_offset, size_t name_length, size_t record_offset,
    size_t record_length, void* user_data )
{
    const uint32_t mcttl = 3600; // seconds
    const uint32_t ucttl = 600; // seconds

    if ( entry != MDNS_ENTRYTYPE_QUESTION )
        return 0;

    const char dns_sd[] = "_services._dns-sd._udp.local.";
    const service_t* service = (const service_t*)user_data;

    /* mdns_string_t fromaddrstr = ip_address_to_string( 
        m_addrbuffer, sizeof( m_addrbuffer ), from, addrlen );*/

    size_t offset = name_offset;
    mdns_string_t name = mdns_string_extract( data, size, &offset,
        service->obj->m_namebuffer, sizeof( service->obj->m_namebuffer ) );

    const char* recordName = 0;
    if ( rtype == MDNS_RECORDTYPE_PTR )
        recordName = "PTR";
    else if ( rtype == MDNS_RECORDTYPE_SRV )
        recordName = "SRV";
    else if ( rtype == MDNS_RECORDTYPE_A )
        recordName = "A";
    else if ( rtype == MDNS_RECORDTYPE_AAAA )
        recordName = "AAAA";
    else if ( rtype == MDNS_RECORDTYPE_ANY )
        recordName = "ANY";
    else
        return 0;

    // printf( "Query %s %.*s\n", recordName, MDNS_STRING_FORMAT( name ) );

    if ( ( name.length == ( sizeof( dns_sd ) - 1 ) )
        && ( strncmp( name.str, dns_sd, sizeof( dns_sd ) - 1 ) == 0 ) )
    {
        if ( ( rtype == MDNS_RECORDTYPE_PTR )
            || ( rtype == MDNS_RECORDTYPE_ANY ) )
        {
            // The PTR query was for the DNS-SD domain, send answer with a PTR record for the
            // service name we advertise, typically on the "<_service-name>._tcp.local." format

            // Answer PTR record reverse mapping "<_service-name>._tcp.local." to
            // "<hostname>.<_service-name>._tcp.local."
            mdns_record_t answer;
            answer.name = name;
            answer.type = MDNS_RECORDTYPE_PTR;
            answer.data.ptr.name = service->service;

            // Send the answer, unicast or multicast depending on flag in query
            uint16_t unicast = ( rclass & MDNS_UNICAST_RESPONSE );
            /* printf( "  --> answer %.*s (%s)\n",
                MDNS_STRING_FORMAT( answer.data.ptr.name ),
                ( unicast ? "unicast" : "multicast" ) ); */

            if ( unicast )
            {
                mdns_query_answer_unicast( sock, from, addrlen, service->obj->m_sendbuffer,
                    sizeof( service->obj->m_sendbuffer ), query_id, (mdns_record_type_t)rtype,
                    name.str, name.length, answer, 0, 0, 0, 0, ucttl );
            }
            else
            {
                mdns_query_answer_multicast( sock, service->obj->m_sendbuffer,
                    sizeof( service->obj->m_sendbuffer ), answer, 0, 0, 0, 0,
                    mcttl );
            }
        }
    }
    else if ( ( name.length == service->service.length )
        && ( strncmp( name.str, service->service.str, name.length ) == 0 ) )
    {
        if ( ( rtype == MDNS_RECORDTYPE_PTR )
            || ( rtype == MDNS_RECORDTYPE_ANY ) )
        {
            // The PTR query was for our service (usually "<_service-name._tcp.local"), answer a PTR
            // record reverse mapping the queried service name to our service instance name
            // (typically on the "<hostname>.<_service-name>._tcp.local." format), and add
            // additional records containing the SRV record mapping the service instance name to our
            // qualified hostname (typically "<hostname>.local.") and port, as well as any IPv4/IPv6
            // address for the hostname as A/AAAA records, and two test TXT records

            // Answer PTR record reverse mapping "<_service-name>._tcp.local." to
            // "<hostname>.<_service-name>._tcp.local."
            mdns_record_t answer = service->record_ptr;

            int additionalTotal = 3 + service->obj->m_txtRecords.size();
            mdns_record_t* additional = new mdns_record_t[additionalTotal] { 0 };
            size_t additionalCount = 0;

            // SRV record mapping "<hostname>.<_service-name>._tcp.local." to
            // "<hostname>.local." with port. Set weight & priority to 0.
            additional[additionalCount++] = service->record_srv;

            // A/AAAA records mapping "<hostname>.local." to IPv4/IPv6 addresses
            if ( service->address_ipv4.sin_family == AF_INET )
                additional[additionalCount++] = service->record_a;
            if ( service->address_ipv6.sin6_family == AF_INET6 )
                additional[additionalCount++] = service->record_aaaa;

            // Setup txt records
            for ( auto& txt : service->txt_records )
            {
                additional[additionalCount++] = txt;
            }

            // Send the answer, unicast or multicast depending on flag in query
            uint16_t unicast = ( rclass & MDNS_UNICAST_RESPONSE );
            /* printf( "  --> answer %.*s (%s)\n",
                MDNS_STRING_FORMAT( service->record_ptr.data.ptr.name ),
                ( unicast ? "unicast" : "multicast" ) );*/

            if ( unicast )
            {
                mdns_query_answer_unicast( sock, from, addrlen, service->obj->m_sendbuffer,
                    sizeof( service->obj->m_sendbuffer ),
                    query_id, (mdns_record_type_t)rtype, name.str,
                    name.length, answer, 0, 0, additional, additionalCount, ucttl );
            }
            else
            {
                mdns_query_answer_multicast( sock, service->obj->m_sendbuffer,
                    sizeof( service->obj->m_sendbuffer ), answer, 0, 0,
                    additional, additionalCount, mcttl );
            }

            delete[] additional;
        }
    }
    else if ( ( name.length == service->service_instance.length )
        && ( strncmp( name.str, service->service_instance.str, name.length ) == 0 ) )
    {
        if ( ( rtype == MDNS_RECORDTYPE_SRV )
            || ( rtype == MDNS_RECORDTYPE_ANY ) )
        {
            // The SRV query was for our service instance (usually
            // "<hostname>.<_service-name._tcp.local"), answer a SRV record mapping the service
            // instance name to our qualified hostname (typically "<hostname>.local.") and port, as
            // well as any IPv4/IPv6 address for the hostname as A/AAAA records, and two test TXT
            // records

            // Answer PTR record reverse mapping "<_service-name>._tcp.local." to
            // "<hostname>.<_service-name>._tcp.local."
            mdns_record_t answer = service->record_srv;

            int additionalTotal = 3 + service->obj->m_txtRecords.size();
            mdns_record_t* additional = new mdns_record_t[additionalTotal] { 0 };
            size_t additionalCount = 0;

            // A/AAAA records mapping "<hostname>.local." to IPv4/IPv6 addresses
            if ( service->address_ipv4.sin_family == AF_INET )
                additional[additionalCount++] = service->record_a;
            if ( service->address_ipv6.sin6_family == AF_INET6 )
                additional[additionalCount++] = service->record_aaaa;

            // Setup txt records
            for ( auto& txt : service->txt_records )
            {
                additional[additionalCount++] = txt;
            }

            // Send the answer, unicast or multicast depending on flag in query
            uint16_t unicast = ( rclass & MDNS_UNICAST_RESPONSE );
            /* printf( "  --> answer %.*s port %d (%s)\n",
                MDNS_STRING_FORMAT( service->record_srv.data.srv.name ), service->port,
                ( unicast ? "unicast" : "multicast" ) );*/

            if ( unicast )
            {
                mdns_query_answer_unicast( sock, from, addrlen,
                    service->obj->m_sendbuffer, sizeof( service->obj->m_sendbuffer ), query_id,
                    (mdns_record_type_t)rtype, name.str, name.length,
                    answer, 0, 0, additional, additionalCount, ucttl );
            }
            else
            {
                mdns_query_answer_multicast( sock, service->obj->m_sendbuffer,
                    sizeof( service->obj->m_sendbuffer ), answer, 0, 0,
                    additional, additionalCount, mcttl );
            }

            delete[] additional;
        }
    }
    else if ( ( name.length == service->hostname_qualified.length )
        && ( strncmp( name.str, service->hostname_qualified.str, name.length ) == 0 ) )
    {
        if ( ( ( rtype == MDNS_RECORDTYPE_A )
                 || ( rtype == MDNS_RECORDTYPE_ANY ) )
            && ( service->address_ipv4.sin_family == AF_INET ) )
        {
            // The A query was for our qualified hostname (typically "<hostname>.local.") and we
            // have an IPv4 address, answer with an A record mappiing the hostname to an IPv4
            // address, as well as any IPv6 address for the hostname, and two test TXT records

            // Answer A records mapping "<hostname>.local." to IPv4 address
            mdns_record_t answer = service->record_a;

            int additionalTotal = 3 + service->obj->m_txtRecords.size();
            mdns_record_t* additional = new mdns_record_t[additionalTotal] { 0 };
            size_t additionalCount = 0;

            // AAAA record mapping "<hostname>.local." to IPv6 addresses
            if ( service->address_ipv6.sin6_family == AF_INET6 )
                additional[additionalCount++] = service->record_aaaa;

            // Setup txt records
            for ( auto& txt : service->txt_records )
            {
                additional[additionalCount++] = txt;
            }

            // Send the answer, unicast or multicast depending on flag in query
            uint16_t unicast = ( rclass & MDNS_UNICAST_RESPONSE );
            /* mdns_string_t addrstr = ip_address_to_string(
                m_addrbuffer, sizeof( m_addrbuffer ), 
                (struct sockaddr*)&service->record_a.data.a.addr,
                sizeof( service->record_a.data.a.addr ) );

            printf( "  --> answer %.*s IPv4 %.*s (%s)\n", 
                MDNS_STRING_FORMAT( service->record_a.name ),
                MDNS_STRING_FORMAT( addrstr ), 
                ( unicast ? "unicast" : "multicast" ) );*/

            if ( unicast )
            {
                mdns_query_answer_unicast( sock, from, addrlen, service->obj->m_sendbuffer,
                    sizeof( service->obj->m_sendbuffer ),
                    query_id, (mdns_record_type_t)rtype, name.str,
                    name.length, answer, 0, 0,
                    additional, additionalCount, ucttl );
            }
            else
            {
                mdns_query_answer_multicast( sock, service->obj->m_sendbuffer,
                    sizeof( service->obj->m_sendbuffer ), answer, 0, 0,
                    additional, additionalCount, mcttl );
            }

            delete[] additional;
        }
        else if ( ( ( rtype == MDNS_RECORDTYPE_AAAA )
                      || ( rtype == MDNS_RECORDTYPE_ANY ) )
            && ( service->address_ipv6.sin6_family == AF_INET6 ) )
        {
            // The AAAA query was for our qualified hostname (typically "<hostname>.local.") and we
            // have an IPv6 address, answer with an AAAA record mappiing the hostname to an IPv6
            // address, as well as any IPv4 address for the hostname, and two test TXT records

            // Answer AAAA records mapping "<hostname>.local." to IPv6 address
            mdns_record_t answer = service->record_aaaa;

            int additionalTotal = 3 + service->obj->m_txtRecords.size();
            mdns_record_t* additional = new mdns_record_t[additionalTotal] { 0 };
            size_t additionalCount = 0;

            // A record mapping "<hostname>.local." to IPv4 addresses
            if ( service->address_ipv4.sin_family == AF_INET )
                additional[additionalCount++] = service->record_a;

            // Setup txt records
            for ( auto& txt : service->txt_records )
            {
                additional[additionalCount++] = txt;
            }

            // Send the answer, unicast or multicast depending on flag in query
            uint16_t unicast = ( rclass & MDNS_UNICAST_RESPONSE );
            /* mdns_string_t addrstr = ip_address_to_string( m_addrbuffer, 
                sizeof( m_addrbuffer ),
                (struct sockaddr*)&service->record_aaaa.data.aaaa.addr,
                sizeof( service->record_aaaa.data.aaaa.addr ) );
            printf( "  --> answer %.*s IPv6 %.*s (%s)\n",
                MDNS_STRING_FORMAT( service->record_aaaa.name ), MDNS_STRING_FORMAT( addrstr ),
                ( unicast ? "unicast" : "multicast" ) );*/

            if ( unicast )
            {
                mdns_query_answer_unicast( sock, from, addrlen,
                    service->obj->m_sendbuffer, sizeof( service->obj->m_sendbuffer ),
                    query_id, (mdns_record_type_t)rtype,
                    name.str, name.length, answer, 0, 0,
                    additional, additionalCount, ucttl );
            }
            else
            {
                mdns_query_answer_multicast( sock, service->obj->m_sendbuffer,
                    sizeof( service->obj->m_sendbuffer ), answer, 0, 0,
                    additional, additionalCount, mcttl );
            }

            delete[] additional;
        }
    }
    return 0;
};

};
