#pragma once
#include <string>

#include <WinSock2.h>

#include <Windows.h>

#include <iphlpapi.h>

namespace zc
{

class InterfaceUtils
{
public:
    static IN_ADDR getInterfaceAddressV4( const std::string& interf );
    static IN6_ADDR getInterfaceAddressV6( const std::string& interf );

private:
    InterfaceUtils() = delete;

    // Memory block returned by this function must be explicitly freed!!!
    static std::pair<IP_ADAPTER_ADDRESSES*, IP_ADAPTER_ADDRESSES*>
    findAdapterByName( const std::string& interf );
};

}
