#pragma once
#include <wx/string.h>

#include <wx/msw/registry.h>

namespace gui
{

class AutorunSetter
{
public:
    explicit AutorunSetter( const wxString& appName, const wxString& command );
    bool isAutorunEnabled() const;
    void disableAutorun();
    void enableAutorun();

private:
    static const wxString KEY_NAME;

    wxRegKey m_runKey;
    wxString m_appName;
    wxString m_command;
};

};
