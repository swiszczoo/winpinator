#include "utils.h"


namespace gui
{

std::unique_ptr<Utils> Utils::s_inst = nullptr;


Utils::Utils()
{
    m_headerFont = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );
    m_headerFont.SetPointSize( 13 );

#ifdef _WIN32
    m_headerColor = wxColour( 0, 51, 153 );
#else
    m_headerFont.MakeBold();
    m_headerColor = wxSystemSettings::GetColour( wxSYS_COLOUR_ACTIVECAPTION );
#endif
}

Utils* Utils::get()
{
    if (!Utils::s_inst)
    {
        Utils::s_inst = std::make_unique<Utils>();
    }

    return Utils::s_inst.get();
}

wxString Utils::makeIntResource( int resource )
{
    return wxString::Format( "#%d", resource );
}

const wxFont& Utils::getHeaderFont() const
{
    return m_headerFont;
}

wxColour Utils::getHeaderColor() const
{
    return m_headerColor;
}

};