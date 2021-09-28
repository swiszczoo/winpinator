#pragma once
#include "../service/remote_info.hpp"

#include <wx/wx.h>

#include <cstdint>
#include <memory>


namespace gui
{

class Utils
{
public:
    static const wxColour GREEN_ACCENT;
    static const wxColour GREEN_BACKGROUND;
    static const wxColour ORANGE_ACCENT;
    static const wxColour ORANGE_BACKGROUND;
    static const wxColour RED_ACCENT;
    static const wxColour RED_BACKGROUND;

    static Utils* get();
    static wxString makeIntResource( int resource );

    const wxFont& getHeaderFont() const;
    wxColour getHeaderColor() const;

    static void drawTextEllipse( wxDC& dc, const wxString& text,
        const wxPoint& pnt, const wxCoord maxWidth );

    static wxString getStatusString( srv::RemoteStatus status );

    static wxIcon extractIconWithSize( const wxIconLocation& loc, wxCoord dim );
    static bool getIconDimensions( HICON hico, SIZE* psiz );

    static wxString fileSizeToString( long long bytes );
    static wxString formatDate( uint64_t timestamp, std::string format );

private:
    // We need a friend to create a unique_ptr
    friend std::unique_ptr<Utils> std::make_unique<Utils>();

    Utils();

    static std::unique_ptr<Utils> s_inst;

    wxFont m_headerFont;
    wxColour m_headerColor;
};

};