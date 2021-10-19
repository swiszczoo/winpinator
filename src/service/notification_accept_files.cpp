#include "notification_accept_files.hpp"

#include <wx/filename.h>
#include <wx/stdpaths.h>

#include <memory>

namespace srv
{

AcceptFilesNotification::AcceptFilesNotification(
    std::string remoteId, int transferId )
    : ToastNotification()
    , m_remoteId( remoteId )
    , m_transferId( transferId )
    , m_senderFullName( L"" )
    , m_singleElementName( L"" )
    , m_isSingleFolder( false )
    , m_elementCount( 0 )
{
}

void AcceptFilesNotification::setSenderFullName( const std::wstring name )
{
    m_senderFullName = name;
}

std::wstring AcceptFilesNotification::getSenderFullName() const
{
    return m_senderFullName;
}

void AcceptFilesNotification::setSingleElementName( const std::wstring name )
{
    m_singleElementName = name;
}

std::wstring AcceptFilesNotification::getSingleElementName() const
{
    return m_singleElementName;
}

void AcceptFilesNotification::setIsSingleFolder( bool single )
{
    m_isSingleFolder = single;
}

bool AcceptFilesNotification::isSingleFolder() const
{
    return m_isSingleFolder;
}

void AcceptFilesNotification::setElementCount( int elementCount )
{
    m_elementCount = elementCount;
}

int AcceptFilesNotification::getElementCount() const
{
    return m_elementCount;
}

WinToastLib::WinToastTemplate AcceptFilesNotification::buildTemplate()
{
    using namespace WinToastLib;

    WinToastTemplate templ( WinToastTemplate::ImageAndText02 );
    templ.setTextField( _( "Incoming files" ), WinToastTemplate::FirstLine );

    wxString secondLine;
    wxFileName iconName( wxStandardPaths::Get().GetUserDataDir(), "a" );
    iconName.AppendDir( "icons" );

    if ( m_isSingleFolder )
    {
        iconName.SetName( "transfer_dx.png" );
        secondLine.Printf( _( "%s is sending you a folder \"%s\"" ),
            m_senderFullName, m_singleElementName );
    }
    else if ( m_elementCount > 1 )
    {
        iconName.SetName( "transfer_ff.png" );
        wxString plural = wxString::Format(
            wxPLURAL( "%d element", "%d elements", (int)m_elementCount ),
            m_elementCount );

        secondLine.Printf( _( "%s is sending you %s" ),
            m_senderFullName, plural );
    }
    else
    {
        iconName.SetName( "transfer_fx.png" );
        secondLine.Printf( _( "%s is sending you a file \"%s\"" ),
            m_senderFullName, m_singleElementName );
    }

    templ.setTextField( secondLine.ToStdWstring(), WinToastTemplate::SecondLine );
    templ.addAction( _( "Accept" ) );
    templ.addAction( _( "Decline" ) );
    templ.setImagePath( iconName.GetFullPath() );

    return templ;
}

WinToastLib::IWinToastHandler* AcceptFilesNotification::instantiateListener()
{
    return new Handler( this );
}

AcceptFilesNotification::Handler::Handler( AcceptFilesNotification* instance )
    : m_remoteId( instance->m_remoteId )
    , m_transferId( instance->m_transferId )
    , m_service( instance->m_service )
{
}

void AcceptFilesNotification::Handler::toastActivated() const
{
    m_service->notifyObservers( [this]( IServiceObserver* observer )
        { observer->onOpenTransferUI( m_remoteId ); } );
}

void AcceptFilesNotification::Handler::toastActivated( int actionIndex ) const
{
    Event evnt;
    TransferData data;
    data.remoteId = m_remoteId;
    data.transferId = m_transferId;

    evnt.eventData.transferData = std::make_shared<TransferData>( data );

    switch ( (Actions)actionIndex )
    {
    case Actions::ACCEPT:
        evnt.type = EventType::ACCEPT_TRANSFER_CLICKED;
        m_service->postEvent( evnt );
        break;

    case Actions::DECLINE:
        evnt.type = EventType::DECLINE_TRANSFER_CLICKED;
        m_service->postEvent( evnt );
        break;
    }
}

void AcceptFilesNotification::Handler::toastDismissed(
    WinToastDismissalReason state ) const
{
}

void AcceptFilesNotification::Handler::toastFailed() const
{
}

};
