#pragma once
#include <cstdint>
#include <map>
#include <string>

namespace zc
{

struct MdnsIpPair
{
    bool valid;
    std::string ipv4;
    std::string ipv6;

    inline bool operator<( const MdnsIpPair& other ) const
    {
        if ( valid < other.valid )
        {
            return true;
        }
        if ( valid > other.valid )
        {
            return false;
        }
        if ( ipv4 < other.ipv4 )
        {
            return true;
        }
        if ( ipv4 > other.ipv4 )
        {
            return false;
        }
        if ( ipv6 < other.ipv6 )
        {
            return true;
        }

        return false;
    }

    inline bool operator==( const MdnsIpPair& other ) const
    {
        return valid == other.valid && ipv4 == other.ipv4
            && ipv6 == other.ipv6;
    }
};

struct MdnsServiceData
{
    // A full service name, consisting of hostname and protocol. Should be
    // uniquely identifying a particular host.
    std::string name;

    // A name of the server in .local. domain
    std::string srvName;

    // Port on which the service is running
    uint16_t port;

    // An IPv4 address of the service. If unavailable, this string is empty.
    std::string ipv4;

    // An IPv6 address of the service. If unavailable, this string is empty.
    std::string ipv6;

    // A map of TXT records
    std::map<std::string, std::string> txtRecords;
};

};
