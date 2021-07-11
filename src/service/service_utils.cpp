#include "service_utils.hpp"

#include <Windows.h>

#define SECURITY_WIN32
#include <security.h>

namespace srv
{

std::string Utils::getHostname()
{
    char buffer[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = MAX_COMPUTERNAME_LENGTH + 1;

    GetComputerNameA( buffer, &size );

    return std::string( buffer );
}

std::wstring Utils::getUserFullName()
{
    ULONG size = 0;
    GetUserNameExW( NameDisplay, NULL, &size );

    if ( size == 0 )
    {
        return L"";
    }

    wchar_t* buffer = new wchar_t[size];
    GetUserNameExW( NameDisplay, buffer, &size );

    std::wstring result( buffer );

    delete[] buffer;

    return result;
}

std::wstring Utils::getUserShortName()
{
    ULONG size = 0;
    GetUserNameW( NULL, &size );

    if ( size == 0 )
    {
        return L"";
    }

    wchar_t* buffer = new wchar_t[size];
    GetUserNameW( buffer, &size );

    std::wstring result( buffer );

    delete[] buffer;

    return result;
}

};
