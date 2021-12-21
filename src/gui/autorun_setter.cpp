#include "autorun_setter.hpp"

#include <wx/log.h>

namespace gui
{

const wxString AutorunSetter::KEY_NAME = "SOFTWARE\\Microsoft\\Windows"
                                         "\\CurrentVersion\\Run";

AutorunSetter::AutorunSetter( const wxString& appName, const wxString& command )
    : m_appName( appName )
    , m_command( command )
{
    m_runKey.SetName( wxRegKey::HKCU, KEY_NAME );
    if ( !m_runKey.Open() )
    {
        wxLogDebug( "Can't open Run registry key" );
    }
}

bool AutorunSetter::isAutorunEnabled() const
{
    if ( !m_runKey.IsOpened() )
    {
        return false;
    }

    if ( !m_runKey.HasValue( m_appName ) )
    {
        return false;
    }

    wxString value;
    m_runKey.QueryValue( m_appName, value );

    return value == m_command;
}

void AutorunSetter::enableAutorun()
{
    if ( !m_runKey.IsOpened() )
    {
        return;
    }

    m_runKey.SetValue( m_appName, m_command );
}

void AutorunSetter::disableAutorun()
{
    if ( !m_runKey.IsOpened() )
    {
        return;
    }

    m_runKey.DeleteValue( m_appName );
}

};
