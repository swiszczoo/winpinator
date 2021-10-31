#include "notification_transfer_failed.hpp"

#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/translation.h>

namespace srv
{

TransferFailedNotification::TransferFailedNotification( std::string remoteId )
    : ToastNotification()
    , m_remoteId( remoteId )
    , m_senderFullName( L"" )
{
}

void TransferFailedNotification::setSenderFullName( const std::wstring name )
{
    m_senderFullName = name;
}

std::wstring TransferFailedNotification::getSenderFullName() const
{
    return m_senderFullName;
}

WinToastLib::WinToastTemplate TransferFailedNotification::buildTemplate()
{
    using namespace WinToastLib;

    WinToastTemplate templ( WinToastTemplate::ImageAndText02 );
    templ.setTextField(
        _( "Transfer failure" ).ToStdWstring(), WinToastTemplate::FirstLine );

    wxString secondLine;
    secondLine.Printf( 
        _( "Some transfers between you and %s have failed unexpectedly!" ),
        m_senderFullName );

    templ.setTextField(
        secondLine.ToStdWstring(), WinToastTemplate::SecondLine );

    wxFileName iconName( wxStandardPaths::Get().GetUserDataDir(), 
        "transfer_fail.png" );
    iconName.AppendDir( "icons" );

    templ.setImagePath( iconName.GetFullPath().ToStdWstring() );

    return templ;
}

WinToastLib::IWinToastHandler* TransferFailedNotification::instantiateListener()
{
    return new Handler( this );
}


TransferFailedNotification::Handler::Handler( 
    TransferFailedNotification* instance )
    : m_remoteId( instance->m_remoteId )
    , m_service( instance->m_service )
{
}

void TransferFailedNotification::Handler::toastActivated() const
{
    m_service->notifyObservers( [this]( IServiceObserver* observer )
        { observer->onOpenTransferUI( m_remoteId ); } );
}

void TransferFailedNotification::Handler::toastActivated( int actionIndex ) const
{
}

void TransferFailedNotification::Handler::toastDismissed(
    WinToastDismissalReason state ) const
{
}

void TransferFailedNotification::Handler::toastFailed() const
{
}

};
