#pragma once
#include <string>

namespace zc
{

struct MdnsIpPair
{
    bool valid;
    std::string ipv4;
    std::string ipv6;
};

};
