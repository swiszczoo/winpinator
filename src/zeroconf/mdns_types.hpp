#pragma once
#include <map>
#include <string>

namespace zc
{

struct MdnsIpPair
{
    bool valid;
    std::string ipv4;
    std::string ipv6;
};

struct MdnsServiceData
{
    // A full service name, consisting of hostname and protocol. Should be
    // uniquely identifying a particular host.
    std::string name;

    // A name of the server in .local. domain
    std::string srvName;

    // An IPv4 address of the service. If unavailable, this string is empty.
    std::string ipv4;

    // An IPv6 address of the service. If unavailable, this string is empty.
    std::string ipv6;

    // A map of TXT records
    std::map<std::string, std::string> txtRecords;
};

};
