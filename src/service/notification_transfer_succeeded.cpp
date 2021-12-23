#include "notification_transfer_succeeded.hpp"

#include <wx/stdpaths.h>

namespace srv
{

TransferSucceededNotification::TransferSucceededNotification( const std::string& remoteId )
    : TransferFailedNotification( remoteId )
{
}

WinToastLib::WinToastTemplate TransferSucceededNotification::buildTemplate()
{
    using namespace WinToastLib;

    WinToastTemplate templ( WinToastTemplate::ImageAndText02 );
    templ.setTextField(
        _( "Transfer success" ).ToStdWstring(), WinToastTemplate::FirstLine );

    wxString secondLine;
    secondLine.Printf(
        _( "Transfer between you and %s has finished successfully!" ),
        m_senderFullName );

    templ.setTextField(
        secondLine.ToStdWstring(), WinToastTemplate::SecondLine );

    wxFileName iconName( wxStandardPaths::Get().GetUserDataDir(),
        "transfer_ok.png" );
    iconName.AppendDir( "icons" );

    templ.setImagePath( iconName.GetFullPath().ToStdWstring() );

    return templ;
}

};
