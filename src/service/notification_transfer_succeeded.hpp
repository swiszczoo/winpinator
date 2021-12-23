#pragma once
#include "notification_transfer_failed.hpp"

namespace srv
{

class TransferSucceededNotification : public TransferFailedNotification
{
public:
    explicit TransferSucceededNotification( const std::string& remoteId );

    virtual WinToastLib::WinToastTemplate buildTemplate() override;
};

};
