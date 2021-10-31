#include "memory_manager.hpp"

#include "../thread_name.hpp"

namespace srv
{

std::unique_ptr<MemoryManager> MemoryManager::s_instance = nullptr;

MemoryManager::MemoryManager()
{
    m_thread = std::thread( std::bind( &MemoryManager::threadMain, this ) );
}

MemoryManager* MemoryManager::getInstance()
{
    if ( !s_instance )
    {
        s_instance = std::make_unique<MemoryManager>();
    }

    return s_instance.get();
}

void MemoryManager::scheduleFreePointer( void* ptr )
{
    Event evnt;
    evnt.pointerToFree = ptr;
    evnt.shutdown = false;

    m_queue.Post( evnt );
}

void MemoryManager::shutdownAndFreeAll()
{
    Event evnt;
    evnt.shutdown = true;
    evnt.pointerToFree = nullptr;

    m_queue.Post( evnt );

    m_thread.join();
}

void MemoryManager::threadMain()
{
    setThreadName( "MemoryManager loop" );

    while ( true )
    {
        Event evnt;
        m_queue.Receive( evnt );

        if ( evnt.shutdown )
        {
            break;
        }

        std::lock_guard<std::mutex> lock( m_mtx );
        m_pointers.erase( evnt.pointerToFree );
    }

    std::lock_guard<std::mutex> lock( m_mtx );
    m_pointers.clear();
}

};

