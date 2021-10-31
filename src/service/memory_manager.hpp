#pragma once
#include <wx/msgqueue.h>

#include <map>
#include <memory>
#include <mutex>

namespace srv
{

class MemoryManager
{
    friend std::unique_ptr<MemoryManager> std::make_unique<MemoryManager>();

private:
    struct Event
    {
        bool shutdown;
        void* pointerToFree;
    };

    explicit MemoryManager();

    static std::unique_ptr<MemoryManager> s_instance;

    std::mutex m_mtx;
    std::map<void*, std::shared_ptr<void>> m_pointers;
    std::thread m_thread;

    wxMessageQueue<Event> m_queue;

    void threadMain();

public:
    static MemoryManager* getInstance();

    template <typename T>
    inline void registerPointer( std::shared_ptr<T> ptr )
    {
        std::lock_guard<std::mutex> lock( m_mtx );
        m_pointers[ptr.get()] = std::static_pointer_cast<void>( ptr );
    }

    void scheduleFreePointer( void* ptr );

    void shutdownAndFreeAll();
};

};
