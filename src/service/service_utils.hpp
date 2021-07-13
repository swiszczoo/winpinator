#pragma once
#include <string>

namespace srv
{

class Utils
{
public:
    // This is a utility class and not a singleton, so we remove the
    // default constructor
    Utils() = delete;

    static std::string getHostname();
    static std::wstring getUserFullName();
    static std::wstring getUserShortName();
    static std::string generateUUID();
};

};
