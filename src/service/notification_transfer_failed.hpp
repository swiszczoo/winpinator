#pragma once
#include "notification.hpp"

#include <string>

namespace srv
{

class TransferFailedNotification : public ToastNotification
{
public:
    explicit TransferFailedNotification( std::string remoteId );

    void setSenderFullName( std::wstring name );
    std::wstring getSenderFullName() const;

    virtual WinToastLib::WinToastTemplate buildTemplate() override;
    virtual WinToastLib::IWinToastHandler* instantiateListener() override;

private:
    class Handler : public WinToastLib::IWinToastHandler
    {
    public:
        explicit Handler( TransferFailedNotification* notification );

    private:
        std::string m_remoteId;
        WinpinatorService* m_service;

        virtual void toastActivated() const override;
        virtual void toastActivated( int actionIndex ) const override;
        virtual void toastDismissed( WinToastDismissalReason state ) const override;
        virtual void toastFailed() const override;
    };
    std::string m_remoteId;
    std::wstring m_senderFullName;
};

};
