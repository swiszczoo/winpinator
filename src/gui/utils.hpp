#pragma once
#include <memory>

#include <wx/wx.h>


namespace gui
{

class Utils
{
public:
    static Utils* get();
    static wxString makeIntResource( int resource );

    const wxFont& getHeaderFont() const;
    wxColour getHeaderColor() const;

private:
    // We need a friend to create a unique_ptr
    friend std::unique_ptr<Utils> std::make_unique<Utils>();

    Utils();

    static std::unique_ptr<Utils> s_inst;

    wxFont m_headerFont;
    wxColour m_headerColor;
};

};