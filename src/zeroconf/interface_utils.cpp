#include "interface_utils.hpp"

#include <ws2ipdef.h>

namespace zc
{

IN_ADDR InterfaceUtils::getInterfaceAddressV4( const std::string& interf )
{
    auto pair = findAdapterByName( interf );
    IP_ADAPTER_ADDRESSES* adapter = pair.first;
    IP_ADAPTER_ADDRESSES* block = pair.second;

    if ( adapter )
    {
        IN_ADDR addr = in4addr_any;

        if ( adapter->Ipv4Enabled )
        {
            IP_ADAPTER_UNICAST_ADDRESS* unicast = adapter->FirstUnicastAddress;
            if ( unicast )
            {
                do
                {
                    SOCKET_ADDRESS sockAddr = unicast->Address;
                    if ( sockAddr.iSockaddrLength == sizeof( SOCKADDR_IN ) )
                    {
                        SOCKADDR_IN* sockAddrIn = (SOCKADDR_IN*)sockAddr.lpSockaddr;
                        addr = sockAddrIn->sin_addr;
                        break;
                    }
                } while ( unicast = unicast->Next );
            }
        }

        free( block );
        return addr;
    }
    else
    {
        return in4addr_any;
    }
}

IN6_ADDR InterfaceUtils::getInterfaceAddressV6( const std::string& interf )
{
    auto pair = findAdapterByName( interf );
    IP_ADAPTER_ADDRESSES* adapter = pair.first;
    IP_ADAPTER_ADDRESSES* block = pair.second;

    if ( adapter )
    {
        IN6_ADDR addr = in6addr_any;

        if ( adapter->Ipv6Enabled )
        {
            IP_ADAPTER_UNICAST_ADDRESS* unicast = adapter->FirstUnicastAddress;
            if ( unicast )
            {
                do
                {
                    SOCKET_ADDRESS sockAddr = unicast->Address;
                    if ( sockAddr.iSockaddrLength == sizeof( SOCKADDR_IN6 ) )
                    {
                        SOCKADDR_IN6* sockAddrIn = (SOCKADDR_IN6*)sockAddr.lpSockaddr;
                        addr = sockAddrIn->sin6_addr;
                        break;
                    }
                } while ( unicast = unicast->Next );
            }
        }

        free( block );
        return addr;
    }
    else
    {
        return in6addr_any;
    }
}

std::pair<IP_ADAPTER_ADDRESSES*, IP_ADAPTER_ADDRESSES*>
InterfaceUtils::findAdapterByName( const std::string& interf )
{
    ULONG size = 0;
    IP_ADAPTER_ADDRESSES* addresses = NULL;
    GetAdaptersAddresses( AF_UNSPEC, NULL, NULL, addresses, &size );

    addresses = (IP_ADAPTER_ADDRESSES*)malloc( size );
    if ( GetAdaptersAddresses( AF_UNSPEC, NULL, NULL, addresses, &size ) != NO_ERROR )
    {
        if ( addresses )
        {
            free( addresses );
        }
        return std::make_pair( nullptr, nullptr );
    }

    IP_ADAPTER_ADDRESSES* current = addresses;
    do
    {
        if ( lstrcmpA( current->AdapterName, interf.c_str() ) == 0 )
        {
            return std::make_pair( current, addresses );
        }
    } while ( current = current->Next );

    free( addresses );
    return std::make_pair( nullptr, nullptr );
}

};
