#pragma once
#include "settings_model.hpp"

#include <wx/app.h>

#include <memory>

class WinpinatorAppBase : public wxApp
{
public:
    SettingsModel m_settings;
    std::unique_ptr<wxConfigBase> m_config;
};

#ifndef NO_DECLARE_APP
wxDECLARE_APP( WinpinatorAppBase );
#endif
