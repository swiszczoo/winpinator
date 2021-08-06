#pragma once
#include <wx/string.h>

#include <string>

#include <Windows.h>

inline void setThreadName( const std::string& name )
{
#ifdef _DEBUG
    HANDLE threadHandle = GetCurrentThread();
    SetThreadDescription( threadHandle, wxString( name ).wc_str() );
#endif
}
