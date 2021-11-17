#include "service_utils.hpp"

#include <wx/platinfo.h>
#include <wx/translation.h>

#include <chrono>

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

std::string Utils::generateUUID()
{
    UUID uid;
    if ( UuidCreate( &uid ) != RPC_S_OK )
    {
        return "";
    }

    char* buffer = 0;
    if ( UuidToStringA( &uid, (RPC_CSTR*)&buffer ) != RPC_S_OK )
    {
        return "";
    }

    if ( buffer )
    {
        std::string result = std::string( buffer );
        RpcStringFreeA( (RPC_CSTR*)&buffer );

        return result;
    }
    
    return "";
}

std::string Utils::getOSVersionString()
{
    const wxPlatformInfo& pinfo = wxPlatformInfo::Get();

    if ( pinfo.GetOperatingSystemId() != wxOperatingSystemId::wxOS_WINDOWS_NT )
    {
        return _( "Unknown OS" ).ToStdString();
    }

    int major = pinfo.GetOSMajorVersion();
    int minor = pinfo.GetOSMinorVersion();

    if ( major == 5 )
    {
        if ( minor == 0 )
            return "Windows 2000";
        if ( minor == 1 || minor == 2 )
            return "Windows XP";
    }

    if ( major == 6 )
    {
        if ( minor == 0 )
            return "Windows Vista";
        if ( minor == 1 )
            return "Windows 7";
        if ( minor == 2 )
            return "Windows 8";
        if ( minor == 3 )
            return "Windows 8.1";
    }

    if ( major == 10 )
    {
        return "Windows 10";
    }

    if ( major == 11 )
    {
        // TODO: ???
        return "Windows 11";
    }


    return "Windows NT";
}

int64_t Utils::getMonotonicTime()
{
    auto stamp = std::chrono::steady_clock::now();
    return stamp.time_since_epoch().count();
}

wxString Utils::makeIntResource( int resource )
{
    return wxString::Format( "#%d", resource );
}

};
