#pragma once

#include <wx/wx.h>

namespace gui
{

class WinpinatorApp : public wxApp
{
public:
    WinpinatorApp();
    bool OnInit() override;

private:
    wxLocale m_locale;
};

};
