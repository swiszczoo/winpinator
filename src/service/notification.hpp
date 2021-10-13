#pragma once
#include "winpinator_service.hpp"

#include <wintoast/wintoastlib.h>

namespace srv
{

class WinpinatorService;

class ToastNotification
{
public:
    explicit ToastNotification();

    void setService( WinpinatorService* service );
    WinpinatorService* getService() const;

    virtual WinToastLib::WinToastTemplate buildTemplate() = 0;
    virtual WinToastLib::IWinToastHandler* instantiateListener() = 0;
    

protected:
    WinpinatorService* m_service;
};

};
