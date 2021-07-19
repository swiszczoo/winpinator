#pragma once
#include "service_observer.hpp"

#include <functional>
#include <mutex>
#include <set>

namespace srv
{

class IServiceObserver;

class ObservableService
{
    friend class IServiceObserver;

public:
    ObservableService();
    ~ObservableService();

    void callForEachObserver( std::function<void( IServiceObserver* )> func );

protected:
    std::set<IServiceObserver*> m_observers;

    // Remember to protect all usages of m_observers with a lock
    // to prevent removing their refs during execution of callbacks
    std::recursive_mutex m_observersMtx;

    void addObserver( IServiceObserver* ptr );
    void removeObserver( IServiceObserver* ptr );
};

};
