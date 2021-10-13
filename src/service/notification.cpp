#include "notification.hpp"

namespace srv
{

ToastNotification::ToastNotification()
    : m_service( nullptr )
{
}

void ToastNotification::setService( WinpinatorService* service )
{
    m_service = service;
}

WinpinatorService* ToastNotification::getService() const
{
    return m_service;
}

};
