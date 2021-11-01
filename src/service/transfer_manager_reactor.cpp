#include "transfer_manager.hpp"

#include "memory_manager.hpp"

#include <wx/filename.h>

namespace srv
{

void TransferManager::StartTransferReactor::setInstance(
    std::shared_ptr<StartTransferReactor> selfPtr )
{
    m_selfPtr = selfPtr;
    MemoryManager::getInstance()->registerPointer( selfPtr );
}

void TransferManager::StartTransferReactor::setRefs(
    std::shared_ptr<grpc::ClientContext> ref1, std::shared_ptr<OpInfo> ref2 )
{
    m_clientCtx = ref1;
    m_request = ref2;
}

void TransferManager::StartTransferReactor::setTransferPtr( TransferOpPtr ptr )
{
    m_transfer = ptr;
    m_useCompression = ptr->useCompression;
}

void TransferManager::StartTransferReactor::setRemoteId(
    const std::string& remoteId )
{
    m_remoteId = remoteId;
}

void TransferManager::StartTransferReactor::setManager( TransferManager* mgr )
{
    m_mgr = mgr;
}

void TransferManager::StartTransferReactor::start()
{
    StartRead( &m_chunk );
    StartCall();
}

void TransferManager::StartTransferReactor::OnDone( const grpc::Status& s )
{
    wxLogDebug( "StartTransferReactor: Transfer completed, code=%d, msg=%s",
        (int)s.error_code(), s.error_message() );

    {
        std::unique_lock<std::mutex> lock( *m_transfer->mutex );

        if ( m_transfer->status == OpStatus::TRANSFERRING
            || m_transfer->status == OpStatus::PAUSED )
        {
            // Wait for one second (in case a StopTransfer rpc arrives)

            m_transfer->meta.sentBytes = m_transfer->totalSize + 1;
            lock.unlock();

            updateProgress( -1 );

            std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
        }
    }

    m_mgr->finishTransfer( m_remoteId, m_transfer->id );

    StartTransferReactor* pointer = m_selfPtr.get();
    m_selfPtr = nullptr;

    MemoryManager::getInstance()->scheduleFreePointer( pointer );
}

void TransferManager::StartTransferReactor::OnReadDone( bool ok )
{
    if ( ok )
    {
        wxLogDebug( "StartTransferReactor: Successfully received file chunk (name=%s, type=%d, mode=%d, size=%d)",
            m_chunk.relative_path(), (int)m_chunk.file_type(), (int)m_chunk.file_mode(),
            (int)m_chunk.chunk().size() );

        updatePaths();

        long long chunkSize = m_chunk.chunk().size();

        if ( m_useCompression )
        {
            std::string decompressed = m_compressor.decompress( m_chunk.chunk() );
            chunkSize = decompressed.size();
        }

        updateProgress( chunkSize );

        // Wait if op is paused
        std::unique_lock<std::mutex> lck( m_transfer->intern.pauseLock->mutex );
        if ( m_transfer->intern.pauseLock->value )
        {
            m_transfer->intern.pauseLock->condVar.wait( lck );
            lck.unlock();
        }

        StartRead( &m_chunk );
    }
}

wxString TransferManager::StartTransferReactor::getAbsolutePath(
    wxString relativePath )
{
    std::wstring outputPath = m_mgr->getOutputPath();
    wxFileName fname( relativePath );
    fname.Normalize( wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE, outputPath );

    return fname.GetFullPath();
}

void TransferManager::StartTransferReactor::updatePaths()
{
    std::wstring lastPath = L"";
    if ( !m_transfer->intern.elements.empty() )
    {
        lastPath = m_transfer->intern.elements.back().relativePath;
    }

    std::wstring currentPath;
    currentPath = wxString::FromUTF8( m_chunk.relative_path() ).ToStdWstring();

    if ( currentPath.find( L':' ) != std::wstring::npos )
    {
        wxLogDebug( "StartTransferReactor: Ignoring invalid path! (%s)",
            currentPath );
    }
    else if ( lastPath != currentPath )
    {
        wxString absolutePath = getAbsolutePath( currentPath );
        wxLogDebug( "StartTransferReactor: Appending new path (%s, absolute: %s)",
            currentPath, absolutePath );

        db::TransferElement element;
        element.absolutePath = absolutePath.ToStdWstring();
        element.relativePath = currentPath;

        if ( m_chunk.file_type() == (int)FileType::REGULAR_FILE )
        {
            element.elementType = db::TransferElementType::FILE;
        }
        else if ( m_chunk.file_type() == (int)FileType::DIRECTORY )
        {
            element.elementType = db::TransferElementType::FOLDER;
        }

        wxFileName fname( element.absolutePath );
        element.elementName = fname.GetFullName().ToStdWstring();

        m_transfer->intern.elements.push_back( element );

        if ( currentPath.find( '/' ) == std::string::npos )
        {
            // We want to count top level elements only

            if ( m_chunk.file_type() == (int)FileType::REGULAR_FILE )
            {
                m_transfer->intern.fileCount++;
            }

            if ( m_chunk.file_type() == (int)FileType::DIRECTORY )
            {
                m_transfer->intern.dirCount++;
            }
        }
    }
}

void TransferManager::StartTransferReactor::updateProgress( long long chunkBytes )
{
    using namespace std::chrono;
    std::lock_guard<std::mutex> lock( m_mgr->m_mtx );

    {
        std::lock_guard<std::mutex> transferLock( *m_transfer->mutex );
        m_transfer->meta.sentBytes += chunkBytes;
    }

    auto currentTime = std::chrono::steady_clock::now();
    long long millisElapsed = duration_cast<milliseconds>(
        currentTime - m_transfer->intern.lastProgressUpdate )
                                  .count();

    if ( millisElapsed > TransferManager::PROGRESS_FREQ_MILLIS || chunkBytes == -1 )
    {
        std::lock_guard<std::mutex> transferLock( *m_transfer->mutex );

        if ( m_transfer->status == OpStatus::TRANSFERRING
            || m_transfer->status == OpStatus::PAUSED )
        {
            m_mgr->sendStatusUpdateNotification( m_remoteId, m_transfer );
            m_transfer->intern.lastProgressUpdate = currentTime;
        }
    }
}

};
