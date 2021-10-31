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
    if ( !m_transfer->intern.relativePaths.empty() )
    {
        lastPath = m_transfer->intern.relativePaths.back();
    }

    std::wstring currentPath;
    currentPath = wxString::FromUTF8( m_chunk.relative_path() ).ToStdWstring();

    if ( lastPath != currentPath )
    {
        wxLogDebug( "StartTransferReactor: Appending new path (%s, absolute: %s)",
            currentPath, getAbsolutePath( currentPath ) );

        m_transfer->intern.relativePaths.push_back( currentPath );
        m_transfer->intern.absolutePaths.push_back(
            getAbsolutePath( currentPath ).ToStdWstring() );
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

    if ( millisElapsed > TransferManager::PROGRESS_FREQ_MILLIS )
    {
        std::lock_guard<std::mutex> transferLock( *m_transfer->mutex );

        m_mgr->sendStatusUpdateNotification( m_remoteId, m_transfer );
        m_transfer->intern.lastProgressUpdate = currentTime;
    }
}

};
