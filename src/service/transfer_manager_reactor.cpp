#include "transfer_manager.hpp"

#include "memory_manager.hpp"

#include <wx/filename.h>

namespace srv
{

const std::wstring TransferManager::StartTransferReactor::ZONE_ID_STREAM
    = L":Zone.Identifier:$DATA";
const int TransferManager::StartTransferReactor::INTRANET_ZONE = 1;

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

void TransferManager::StartTransferReactor::setCanOverwrite( bool canOverwrite )
{
    m_canOverwrite = canOverwrite;
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

    m_filePtr.Close();
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

        std::string original = m_chunk.chunk();
        long long chunkSize = original.size();
        std::string decompressed;

        if ( m_useCompression )
        {
            decompressed = m_compressor.decompress( original );
            chunkSize = decompressed.size();
        }

        updateProgress( chunkSize );
        processData( m_useCompression ? decompressed : original );

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
    wxLogNull null;

    std::wstring outputPath = m_mgr->getOutputPath();
    wxFileName fname( relativePath );
    fname.Normalize( wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE, outputPath );

    if ( !fname.IsOk() )
    {
        return wxEmptyString;
    }

    wxString fullPath = fname.GetFullPath();

    if ( !fullPath.Contains( outputPath ) )
    {
        // We've been probably tricked to save outside the output path
        return wxEmptyString;
    }

    return fullPath;
}

void TransferManager::StartTransferReactor::updatePaths()
{
    wxLogNull logNull;

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

        if ( absolutePath.empty() )
        {
            wxLogDebug( "StartTransferReactor: Ignoring invalid path 2! (%s)",
                currentPath );
        }
        else
        {
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

            std::lock_guard<std::mutex> guard( *m_transfer->mutex );
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

void TransferManager::StartTransferReactor::processData( const std::string& chunk )
{
    std::string relativePath = m_chunk.relative_path();
    wxString absolutePath = getAbsolutePath( wxString::FromUTF8( relativePath ) );

    if ( absolutePath.empty() )
    {
        return;
    }

    wxLogNull logNull;

    if ( m_chunk.file_type() == (int)FileType::REGULAR_FILE )
    {
        if ( relativePath != m_filePtrPath )
        {
            m_filePtr.Close();
            m_filePtrPath = relativePath;

            if ( !m_filePtr.Create( absolutePath, m_canOverwrite, wxS_DEFAULT ) )
            {
                wxLogDebug( "StartTransferReactor: TRANSFER FAILED! Can't create file %s", absolutePath );
                failOp();
                return;
            }

            if ( m_mgr->getPreserveZoneInfo() )
            {
                writeZoneStream( absolutePath );
            }
        }

        m_filePtr.Write( chunk.data(), chunk.size() );
    }
    else if ( m_chunk.file_type() == (int)FileType::DIRECTORY )
    {
        if ( !wxDirExists( absolutePath ) )
        {
            if ( wxFileExists( absolutePath ) )
            {
                wxRemoveFile( absolutePath );
            }

            if ( !wxMkdir( absolutePath ) )
            {
                wxLogDebug( "StartTransferReactor: TRANSFER FAILED! Can't create directory %s", absolutePath );
                failOp();
                return;
            }
        }
    }
}

void TransferManager::StartTransferReactor::failOp()
{
    m_mgr->requestStopTransfer( m_remoteId, m_transfer->id, true );
}

bool TransferManager::StartTransferReactor::writeZoneStream(
    const wxString& absolutePath )
{
    wxLogNull logNull;

    wxString path = wxString::Format( "%s%s", absolutePath, ZONE_ID_STREAM );

    wxFile zoneFile;
    if ( zoneFile.Create( path, true, wxS_DEFAULT ) )
    {
        wxString content = wxString::Format( 
            "[ZoneTransfer]\nZoneId=%d\n", (int)INTRANET_ZONE );
        zoneFile.Write( content, wxConvLocal );

        return true;
    }

    return false;
}

};
