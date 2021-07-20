#include "mdns_client.hpp"

#include <stdlib.h>

namespace zc
{

MdnsClient::MdnsClient( const std::string& serviceType )
    : m_srvType( serviceType )
    , m_running( false )
    , m_addListener( []( const MdnsServiceData& dummy ) {} )
    , m_removeListener( []( const std::string& dummy ) {} )
    , m_lastServiceName( "" )
    , m_lastServiceTtl( 0 )
    , m_ignoredHost( "" )
    , m_serviceAddressIpv4( { 0 } )
    , m_serviceAddressIpv6( { 0 } )
    , m_hasIpv4( false )
    , m_hasIpv6( false )
    , m_socketToKill( 0 )
    , m_addrbuffer( "" )
    , m_entrybuffer( "" )
    , m_namebuffer( "" )
    , m_sendbuffer( "" )
    , m_txtbuffer()
{
}

MdnsClient::~MdnsClient()
{
    if ( m_running )
    {
        stopListening();
    }

    if ( m_worker.joinable() )
    {
        m_worker.join();
    }
}

void MdnsClient::setOnAddServiceListener( AddListenerType listener )
{
    std::lock_guard<std::mutex> lock( m_mtListeners );

    m_addListener = listener;
}

void MdnsClient::setOnRemoveServiceListener( RemoveListenerType listener )
{
    std::lock_guard<std::mutex> lock( m_mtListeners );

    m_removeListener = listener;
}

void MdnsClient::setIgnoredHostname( const std::string& ignored )
{
    std::lock_guard<std::recursive_mutex> guard( m_mtIgnored );

    m_ignoredHost = ignored;
}

std::string MdnsClient::getIgnoredHostname()
{
    std::lock_guard<std::recursive_mutex> guard( m_mtIgnored );

    return m_ignoredHost;
}

void MdnsClient::startListening()
{
    if ( m_running )
    {
        return;
    }

    if ( m_worker.joinable() )
    {
        m_worker.join();
    }

    m_worker = std::thread( [this]() -> int {
        return workerImpl();
    } );
}

void MdnsClient::stopListening()
{
    std::unique_lock<std::mutex> lock( m_mtRunning );

    if ( !m_running )
    {
        return;
    }

    m_running = false;

    unlockThread();

    lock.unlock();

    if ( m_worker.joinable() )
    {
        m_worker.join();
    }
}

bool MdnsClient::isListening() const
{
    return m_running;
}

void MdnsClient::repeatQuery()
{
    int sockets[32];
    int query_id[32];
    int num_sockets = openClientSockets( sockets,
        sizeof( sockets ) / sizeof( sockets[0] ), MDNS_PORT );

    size_t capacity = 65536;
    void* buffer = malloc( capacity );

    for ( int isock = 0; isock < num_sockets; ++isock )
    {
        query_id[isock] = mdns_query_send( sockets[isock],
            MDNS_RECORDTYPE_PTR, m_srvType.c_str(), m_srvType.length(),
            buffer, capacity, 0 );

        if ( query_id[isock] < 0 )
        {
            char error[256];
            strerror_s( error, errno );
            printf( "Failed to send mDNS query: %s\n", error );
        }
    }

    free( buffer );

    for ( int isock = 0; isock < num_sockets; ++isock )
    {
        mdns_socket_close( sockets[isock] );
    }
}

int MdnsClient::workerImpl()
{
    sendMdnsQuery( m_srvType.c_str(), MDNS_RECORDTYPE_ANY );
    return EXIT_SUCCESS;
}

bool MdnsClient::isValidService(const std::string& name)
{
    const MdnsServiceData& data = m_activeServices.at( name );

    return !data.ipv4.empty() || !data.ipv6.empty();
}

void MdnsClient::processEvents()
{
    std::unique_lock<std::recursive_mutex> lock( m_mtIgnored );
    std::string ignoreStr = m_ignoredHost + '.' + m_srvType;
    lock.unlock();

    int counter = m_addedServices.size();
    while ( counter > 0 )
    {
        const std::string name = m_addedServices.front();

        if ( m_activeServices.find( name ) != m_activeServices.end() 
            && isValidService( name ) )
        {
            if ( name != ignoreStr )
            {
                m_addListener( m_activeServices[name] );
            }
        }
        else
        {
            m_addedServices.push( name );
        }

        m_addedServices.pop();
        counter--;
    }

    while ( !m_removedServices.empty() )
    {
        const std::string name = m_removedServices.front();

        if ( name != ignoreStr )
        {
            m_removeListener( name );
        }

        m_removedServices.pop();
    }
}

void MdnsClient::unlockThread()
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
    sockaddr_in6 addr6;
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

mdns_string_t MdnsClient::ipv4AddressToString( char* buffer, size_t capacity,
    const struct sockaddr_in* addr, size_t addrlen )
{
    char host[NI_MAXHOST] = { 0 };
    char service[NI_MAXSERV] = { 0 };
    int ret = getnameinfo( (const struct sockaddr*)addr, (socklen_t)addrlen, host, NI_MAXHOST,
        service, NI_MAXSERV, NI_NUMERICSERV | NI_NUMERICHOST );
    int len = 0;
    if ( ret == 0 )
    {
        if ( addr->sin_port != 0 )
            len = snprintf( buffer, capacity, "%s:%s", host, service );
        else
            len = snprintf( buffer, capacity, "%s", host );
    }
    if ( len >= (int)capacity )
        len = (int)capacity - 1;
    mdns_string_t str;
    str.str = buffer;
    str.length = len;
    return str;
}

mdns_string_t MdnsClient::ipv6AddressToString( char* buffer, size_t capacity,
    const struct sockaddr_in6* addr, size_t addrlen )
{
    char host[NI_MAXHOST] = { 0 };
    char service[NI_MAXSERV] = { 0 };
    int ret = getnameinfo( (const struct sockaddr*)addr, (socklen_t)addrlen, host, NI_MAXHOST,
        service, NI_MAXSERV, NI_NUMERICSERV | NI_NUMERICHOST );
    int len = 0;
    if ( ret == 0 )
    {
        if ( addr->sin6_port != 0 )
            len = snprintf( buffer, capacity, "[%s]:%s", host, service );
        else
            len = snprintf( buffer, capacity, "%s", host );
    }
    if ( len >= (int)capacity )
        len = (int)capacity - 1;
    mdns_string_t str;
    str.str = buffer;
    str.length = len;
    return str;
}

mdns_string_t MdnsClient::ipAddressToString( char* buffer, size_t capacity,
    const struct sockaddr* addr, size_t addrlen )
{
    if ( addr->sa_family == AF_INET6 )
    {
        return ipv6AddressToString( buffer, capacity,
            (const struct sockaddr_in6*)addr, addrlen );
    }
    return ipv4AddressToString( buffer, capacity,
        (const struct sockaddr_in*)addr, addrlen );
}

int MdnsClient::openClientSockets( int* sockets, int max_sockets, int port )
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
                    if ( numSockets < max_sockets )
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
                    if ( numSockets < max_sockets )
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

int MdnsClient::sendMdnsQuery( const char* service, int record )
{
    int sockets[32];
    int query_id[32];
    int num_sockets = openClientSockets( sockets,
        sizeof( sockets ) / sizeof( sockets[0] ), MDNS_PORT );

    if ( num_sockets <= 0 )
    {
        printf( "Failed to open any client sockets\n" );
        return -1;
    }
    //printf( "Opened %d socket%s for mDNS query\n", num_sockets, num_sockets ? "s" : "" );

    size_t capacity = 65536;
    void* buffer = malloc( capacity );
    void* user_data = this;
    size_t records;

    const char* record_name = "PTR";
    if ( record == MDNS_RECORDTYPE_SRV )
        record_name = "SRV";
    else if ( record == MDNS_RECORDTYPE_A )
        record_name = "A";
    else if ( record == MDNS_RECORDTYPE_AAAA )
        record_name = "AAAA";
    else
        record = MDNS_RECORDTYPE_PTR;

    //printf( "Sending mDNS query: %s %s\n", service, record_name );
    for ( int isock = 0; isock < num_sockets; ++isock )
    {
        query_id[isock] = mdns_query_send( sockets[isock],
            (mdns_record_type_t)record, service, strlen( service ),
            buffer, capacity, 0 );

        if ( query_id[isock] < 0 )
        {
            char error[256];
            strerror_s( error, errno );
            printf( "Failed to send mDNS query: %s\n", error );
        }
    }

    // This is a simple implementation that loops for 5 seconds or as long as we get replies
    int res;
    //printf( "Reading mDNS query replies\n" );
    do
    {
        int nfds = 0;
        fd_set readfs;
        FD_ZERO( &readfs );
        for ( int isock = 0; isock < num_sockets; ++isock )
        {
            if ( sockets[isock] >= nfds )
                nfds = sockets[isock] + 1;
            FD_SET( sockets[isock], &readfs );
        }

        records = 0;

        m_socketToKill = sockets[0];

        res = select( nfds, &readfs, 0, 0, 0 );
        if ( res > 0 )
        {
            for ( int isock = 0; isock < num_sockets; ++isock )
            {
                if ( FD_ISSET( sockets[isock], &readfs ) )
                {
                    records += mdns_query_recv( sockets[isock], buffer, capacity,
                        MdnsClient::queryCallback, user_data, query_id[isock] );

                    processEvents();
                }
                FD_SET( sockets[isock], &readfs );
            }
        }
    } while ( res > 0 );

    free( buffer );

    for ( int isock = 0; isock < num_sockets; ++isock )
        mdns_socket_close( sockets[isock] );
    //printf( "Closed socket%s\n", num_sockets ? "s" : "" );

    return 0;
}

int MdnsClient::queryCallback( int sock, const struct sockaddr* from,
    size_t addrlen, mdns_entry_type_t entry, uint16_t query_id, uint16_t rtype,
    uint16_t rclass, uint32_t ttl, const void* data,
    size_t size, size_t name_offset, size_t name_length, size_t record_offset,
    size_t record_length, void* user_data )
{
    MdnsClient* obj = (MdnsClient*)user_data;

    mdns_string_t fromaddrstr = obj->ipAddressToString( obj->m_addrbuffer,
        sizeof( obj->m_addrbuffer ), from, addrlen );

    const char* entrytype = ( entry == MDNS_ENTRYTYPE_ANSWER )
        ? "answer"
        : ( ( entry == MDNS_ENTRYTYPE_AUTHORITY )
                ? "authority"
                : "additional" );

    mdns_string_t entrystr = mdns_string_extract( data, size, &name_offset,
        obj->m_entrybuffer, sizeof( obj->m_entrybuffer ) );

    std::string entryString( entrystr.str, entrystr.length );

    if ( rtype == MDNS_RECORDTYPE_PTR )
    {
        mdns_string_t namestr = mdns_record_parse_ptr( data, size, record_offset,
            record_length, obj->m_namebuffer, sizeof( obj->m_namebuffer ) );

        std::string name( namestr.str, namestr.length );

        if ( entryString == obj->m_srvType )
        {
            if ( ttl > 0 )
            {
                if ( obj->m_activeServices.find( name ) == obj->m_activeServices.end() )
                {

                    MdnsServiceData data = MdnsServiceData();
                    data.name = name;

                    obj->m_activeServices[name] = data;

                    // Remember the name of this service to send an event later
                    obj->m_addedServices.push( name );
                }
            }
            else
            {
                obj->m_activeServices.erase( name );

                if ( obj->m_removedServices.empty() 
                    || obj->m_removedServices.back() != name )
                {
                    obj->m_removedServices.push( name );
                }
            }
        }
    }
    else if ( rtype == MDNS_RECORDTYPE_SRV )
    {
        mdns_record_srv_t srv = mdns_record_parse_srv( data, size, record_offset,
            record_length, obj->m_namebuffer, sizeof( obj->m_namebuffer ) );
        
        std::string entry( entrystr.str, entrystr.length );
        std::string srvName( srv.name.str, srv.name.length );

        if ( obj->m_activeServices.find( entry ) 
            != obj->m_activeServices.end() )
        {
            obj->m_activeServices[entry].srvName = srvName;
            obj->m_activeServices[entry].port = srv.port;

            if ( obj->m_aRecords.find(srvName) != obj->m_aRecords.end() )
            {
                obj->m_activeServices[entry].ipv4 = obj->m_aRecords[srvName];
            }

            if ( obj->m_aaaaRecords.find(srvName) != obj->m_aaaaRecords.end() )
            {
                obj->m_activeServices[entry].ipv6 = obj->m_aaaaRecords[srvName];
            }
        }
    }
    else if ( rtype == MDNS_RECORDTYPE_A )
    {
        struct sockaddr_in addr;
        mdns_record_parse_a( data, size, record_offset, record_length, &addr );
        mdns_string_t addrstr = obj->ipv4AddressToString( obj->m_namebuffer,
            sizeof( obj->m_namebuffer ), &addr, sizeof( addr ) );

        if ( ttl > 0 )
        {
            std::string servName( entrystr.str, entrystr.length );
            std::string ipv4( addrstr.str, addrstr.length );
            obj->m_aRecords[servName] = ipv4;
        }
        else
        {
            std::string servName( entrystr.str, entrystr.length );
            std::string ipv4( addrstr.str, addrstr.length );

            if ( obj->m_aRecords.find( servName ) != obj->m_aRecords.end() )
            {
                if ( obj->m_aRecords.at( servName ) == ipv4 )
                {
                    obj->m_aRecords.erase( servName );
                }
            }
        }
    }
    else if ( rtype == MDNS_RECORDTYPE_AAAA )
    {
        struct sockaddr_in6 addr;
        mdns_record_parse_aaaa( data, size, record_offset, record_length, &addr );
        mdns_string_t addrstr = obj->ipv6AddressToString( obj->m_namebuffer,
            sizeof( obj->m_namebuffer ), &addr, sizeof( addr ) );

        if ( ttl > 0 )
        {
            std::string servName( entrystr.str, entrystr.length );
            std::string ipv6( addrstr.str, addrstr.length );
            obj->m_aaaaRecords[servName] = ipv6;
        }
        else
        {
            std::string servName( entrystr.str, entrystr.length );
            std::string ipv6( addrstr.str, addrstr.length );

            if ( obj->m_aaaaRecords.find( servName ) != obj->m_aaaaRecords.end() )
            {
                if ( obj->m_aaaaRecords.at( servName ) == ipv6 )
                {
                    obj->m_aaaaRecords.erase( servName );
                }
            }
        }
    }
    else if ( rtype == MDNS_RECORDTYPE_TXT )
    {
        size_t parsed = mdns_record_parse_txt( data, size, record_offset,
            record_length, obj->m_txtbuffer,
            sizeof( obj->m_txtbuffer ) / sizeof( mdns_record_txt_t ) );

        if ( obj->m_activeServices.find( entryString )
            != obj->m_activeServices.end() )
        {
            auto& txts = obj->m_activeServices[entryString].txtRecords;

            for ( size_t itxt = 0; itxt < parsed; ++itxt )
            {
                if ( obj->m_txtbuffer[itxt].value.length )
                {
                    std::string key( obj->m_txtbuffer[itxt].key.str,
                        obj->m_txtbuffer[itxt].key.length );
                    std::string value( obj->m_txtbuffer[itxt].value.str,
                        obj->m_txtbuffer[itxt].value.length );

                    txts[key] = value;
                }
                else
                {
                    std::string key( obj->m_txtbuffer[itxt].key.str,
                        obj->m_txtbuffer[itxt].key.length );

                    txts[key] = "";
                }
            }
        }
    }
    else
    {
        /* printf( "%.*s : %s %.*s type %u rclass 0x%x ttl %u length %d\n",
            MDNS_STRING_FORMAT( fromaddrstr ), entrytype,
            MDNS_STRING_FORMAT( entrystr ), rtype,
            rclass, ttl, (int)record_length ); */
    }
    return 0;
}

};
