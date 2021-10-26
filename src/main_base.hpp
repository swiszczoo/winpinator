#pragma once
#include "settings_model.hpp"

#include <wx/app.h>

#include <memory>

class WinpinatorAppBase : public wxApp
{
public:
    SettingsModel m_settings;
    std::unique_ptr<wxConfigBase> m_config;
    wxLocale m_locale;
};

#ifndef NO_DECLARE_APP
inline WinpinatorAppBase& GetApp()
{
    return *static_cast<WinpinatorAppBase*>( wxApp::GetInstance() );
}
#endif
