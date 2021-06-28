#include "utils.h"


namespace gui
{

wxString Utils::makeIntResource( int resource )
{
    return wxString::Format( "#%d", resource );
}

};