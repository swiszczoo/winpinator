#pragma once
#include <wx/string.h>

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
    static std::string getOSVersionString();
    static int64_t getMonotonicTime();

    static wxString makeIntResource( int resource );

private:
    static std::wstring getFullNameFromGetUserNameEx();
    static std::wstring getFullNameFromNetUserGetInfo();
};

};
