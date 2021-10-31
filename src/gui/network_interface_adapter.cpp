#include "network_interface_adapter.hpp"

#include <winsock2.h>

#include <Windows.h>

#include <iphlpapi.h>

namespace gui
{

NetworkInterfaceAdapter::NetworkInterfaceAdapter()
    : m_dataReady( false )
{
}

std::vector<NetworkInterfaceAdapter::InetInfo>
NetworkInterfaceAdapter::getAllInterfaces()
{
    if ( !m_dataReady )
    {
        loadData();
    }

    return m_data;
}

std::string NetworkInterfaceAdapter::getDefaultInterfaceName()
{
    for ( const InetInfo& info : m_data )
    {
        if ( info.defaultInterface )
        {
            return info.name;
        }
    }

    return "";
}

void NetworkInterfaceAdapter::loadData()
{
    m_data.clear();

    ULONG size = 0;
    IP_ADAPTER_ADDRESSES* addresses = NULL;
    GetAdaptersAddresses( AF_UNSPEC, NULL, NULL, addresses, &size );

    addresses = (IP_ADAPTER_ADDRESSES*)malloc( size );
    if ( GetAdaptersAddresses( AF_UNSPEC, NULL, NULL, addresses, &size ) == NO_ERROR )
    {
        IP_ADAPTER_ADDRESSES* current = addresses;
        do
        {
            InetInfo info;
            info.defaultInterface = false;
            info.name = std::string( current->AdapterName );

            if ( lstrlenW( current->Description ) > 0 )
            {
                info.displayName = std::wstring( current->FriendlyName )
                    + L" (" + std::wstring( current->Description ) + L")";
            }
            else
            {
                info.displayName = std::wstring( current->FriendlyName );
            }
            info.metricV4 = current->Ipv4Metric;
            info.metricV6 = current->Ipv6Metric;

            m_data.push_back( info );
        } while ( current = current->Next );
    }

    if ( addresses )
    {
        free( addresses );
    }

    int i = 0;
    int smallestId = 0;
    int smallestMetric = INT_MAX;
    for ( InetInfo& info : m_data )
    {
        if ( info.metricV4 < smallestMetric )
        {
            smallestId = i;
            smallestMetric = info.metricV4;
        }
        i++;
    }

    m_data[smallestId].defaultInterface = true;

    m_dataReady = true;
}

};
