#pragma once
#include "notification.hpp"

#include <string>

namespace srv
{

class AcceptFilesNotification : public ToastNotification
{
public:
    explicit AcceptFilesNotification( std::string remoteId, int transferId );

    void setSenderFullName( std::wstring name );
    std::wstring getSenderFullName() const;

    void setSingleElementName( std::wstring name );
    std::wstring getSingleElementName() const;

    void setIsSingleFolder( bool single );
    bool isSingleFolder() const;

    void setElementCount( int elementCount );
    int getElementCount() const;

    virtual WinToastLib::WinToastTemplate buildTemplate() override;
    virtual WinToastLib::IWinToastHandler* instantiateListener() override;

private:
    class Handler : public WinToastLib::IWinToastHandler
    {
    public:
        Handler( AcceptFilesNotification* notification );

    private:
        enum class Actions
        {
            ACCEPT = 0,
            DECLINE
        };

        std::string m_remoteId;
        int m_transferId;
        WinpinatorService* m_service;

        virtual void toastActivated() const override;
        virtual void toastActivated( int actionIndex ) const override;
        virtual void toastDismissed( WinToastDismissalReason state ) const override;
        virtual void toastFailed() const override;
    };

    std::string m_remoteId;
    int m_transferId;

    std::wstring m_senderFullName;
    std::wstring m_singleElementName;
    bool m_isSingleFolder;
    int m_elementCount;
};

};
