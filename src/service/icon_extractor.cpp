#include "icon_extractor.hpp"

#include "../../win32/resource.h"
#include "service_utils.hpp"

#include <wx/filename.h>
#include <wx/stdpaths.h>

namespace srv
{

void IconExtractor::extractIcons()
{
    wxFileName fname( wxStandardPaths::Get().GetUserDataDir(), "icons" );
    const wxString iconDir = fname.GetFullPath();

    if ( !fname.Exists() )
    {
        wxMkDir( iconDir );
    }

    extractSingleIcon( iconDir, "transfer_dd.png", IDB_TRANSFER_DIR_DIR );
    extractSingleIcon( iconDir, "transfer_df.png", IDB_TRANSFER_DIR_FILE );
    extractSingleIcon( iconDir, "transfer_dx.png", IDB_TRANSFER_DIR_X );
    extractSingleIcon( iconDir, "transfer_ff.png", IDB_TRANSFER_FILE_FILE );
    extractSingleIcon( iconDir, "transfer_fx.png", IDB_TRANSFER_FILE_X );
    extractSingleIcon( iconDir, "transfer_fail.png", IDB_TRANSFER_FAILED );
}

void IconExtractor::extractSingleIcon( const wxString& iconDir,
    const wxString& filename, int resourceId )
{
    wxFileName fname( iconDir, filename );

    if ( fname.FileExists() )
    {
        return;
    }

    wxBitmap bmp;
    bmp.LoadFile( Utils::makeIntResource( resourceId ), 
        wxBITMAP_TYPE_PNG_RESOURCE );

    wxImage img = bmp.ConvertToImage();
    img.SaveFile( fname.GetFullPath(), wxBITMAP_TYPE_PNG );
}

};
