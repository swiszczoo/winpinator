#pragma once
#include <wx/wx.h>

namespace srv
{

class IconExtractor
{
public:
    static void extractIcons();

private:
    static void extractSingleIcon( const wxString& iconDir, 
        const wxString& filename, int resourceId );
};

};
