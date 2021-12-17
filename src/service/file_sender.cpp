#include "file_sender.hpp"

#include <wx/filename.h>
#include <wx/log.h>

#ifdef _DEBUG
#define WXLOGNULL wxLogNull __lognull
#else
#define WXLOGNULL
#endif

namespace srv
{

const long long FileSender::PROGRESS_FREQ_MILLIS = 250;
const long long FileSender::FILE_CHUNK_SIZE = 1024 * 1024; // 1 MB
const std::vector<std::wstring> FileSender::EXECUTABLE_EXTENSIONS = {
    L"py",
    L"sh"
};

FileSender::FileSender( std::shared_ptr<TransferOp> transfer,
    grpc::ServerWriter<FileChunk>* writer )
    : m_transfer( transfer )
    , m_writer( writer )
    , m_filePerms()
    , m_execPerms()
    , m_dirPerms()
    , m_compressLvl( 0 )
    , m_compressor()
{
}

void FileSender::setCompressionLevel( int level )
{
    m_compressLvl = level;
}

int FileSender::getCompressionLevel() const
{
    return m_compressLvl;
}

void FileSender::setUnixPermissionMasks( int file, int executable, int directory )
{
    m_filePerms.loadFromChmod( file );
    m_execPerms.loadFromChmod( executable );
    m_dirPerms.loadFromChmod( directory );
}

int FileSender::getUnixFilePermissionMask()
{
    return m_filePerms.convertToChmod();
}

int FileSender::getUnixExecutablePermissionMask()
{
    return m_execPerms.convertToChmod();
}

int FileSender::getUnixDirectoryPermissionMask()
{
    return m_dirPerms.convertToChmod();
}

bool FileSender::transferFiles( std::function<void()> onUpdate )
{
    int size = m_transfer->intern.relativePaths->size();
    std::wstring root = m_transfer->intern.rootDir;

    for ( int i = 0; i < size; i++ )
    {
        if ( checkOpFailed() )
        {
            return false;
        }

        if ( !sendSingleEntity( root,
                 m_transfer->intern.relativePaths->at( i ), onUpdate ) )
        {
            return false;
        }
    }

    updateProgress( -1, onUpdate );
    return true;
}

bool FileSender::sendSingleEntity( const std::wstring& root,
    const std::wstring& relativePath, std::function<void()>& onUpdate )
{
    wxString fullPath = root + "\\" + relativePath;

    if ( wxFileExists( fullPath ) )
    {
        return sendSingleFile( root, relativePath, onUpdate );
    }
    else if ( wxDirExists( fullPath ) )
    {
        return sendSingleDirectory( root, relativePath, onUpdate );
    }

    return false;
}

bool FileSender::sendSingleFile( const std::wstring& root,
    const std::wstring& relativePath, std::function<void()>& onUpdate )
{
    wxString relativePathStr = relativePath;
    relativePathStr.Replace( '\\', '/' );
    wxLogDebug( "FileSender: Sending file %s", relativePathStr );

    WXLOGNULL;

    wxString fullPath = root + "\\" + relativePath;
    wxFile file( fullPath, wxFile::read );

    if ( !file.IsOpened() )
    {
        return false;
    }

    int readCount = 0;
    bool contentsDetermined = false;
    UnixPermissions permissions;
    do
    {
        std::unique_ptr<char[]> buf = std::make_unique<char[]>( FILE_CHUNK_SIZE );
        readCount = file.Read( buf.get(), FILE_CHUNK_SIZE );

        std::string data( buf.get(), readCount );

        if ( !contentsDetermined )
        {
            if ( isFileExecutable( data, relativePath ) )
            {
                permissions = m_execPerms;
            }
            else
            {
                permissions = m_filePerms;
            }

            contentsDetermined = true;
        }

        FileChunk fileChunk;

        if ( m_compressLvl > 0 )
        {
            fileChunk.set_chunk( m_compressor.compress( data, m_compressLvl ) );
        }
        else
        {
            fileChunk.set_chunk( data );
        }

        fileChunk.set_relative_path( relativePathStr.ToUTF8() );
        fileChunk.set_file_type( (int)FileType::REGULAR_FILE );
        fileChunk.set_symlink_target( "" );
        fileChunk.set_file_mode( permissions.convertToDecimal() );

        waitIfPaused();

        m_writer->Write( fileChunk );

        if ( readCount > 0 )
        {
            updateProgress( readCount, onUpdate );
        }
    } while ( readCount > 0 );

    return true;
}

bool FileSender::sendSingleDirectory( const std::wstring& root,
    const std::wstring& relativePath, std::function<void()>& onUpdate )
{
    wxString relativePathStr = relativePath;
    relativePathStr.Replace( '\\', '/' );
    wxLogDebug( "FileSender: Sending directory %s", relativePathStr );

    WXLOGNULL;

    FileChunk dirChunk;
    dirChunk.set_relative_path( relativePathStr.ToUTF8() );
    dirChunk.set_file_type( (int)FileType::DIRECTORY );
    dirChunk.set_symlink_target( "" );
    dirChunk.set_chunk( "" );
    dirChunk.set_file_mode( m_dirPerms.convertToDecimal() );

    waitIfPaused();

    m_writer->Write( dirChunk );
    return true;
}

bool FileSender::checkOpFailed()
{
    std::lock_guard<std::mutex> guard( *m_transfer->mutex );

    return m_transfer->status == OpStatus::FAILED
        || m_transfer->status == OpStatus::FAILED_UNRECOVERABLE
        || m_transfer->status == OpStatus::STOPPED_BY_RECEIVER
        || m_transfer->status == OpStatus::STOPPED_BY_SENDER;
}

void FileSender::updateProgress( long long chunkBytes,
    std::function<void()>& onUpdate )
{
    using namespace std::chrono;

    {
        std::lock_guard<std::mutex> transferLock( *m_transfer->mutex );

        if ( chunkBytes >= 0 )
        {
            m_transfer->meta.sentBytes += chunkBytes;
        } 
        else 
        {
            m_transfer->meta.sentBytes = m_transfer->totalSize;
        }
    }

    auto currentTime = std::chrono::steady_clock::now();
    long long millisElapsed = duration_cast<milliseconds>(
        currentTime - m_transfer->intern.lastProgressUpdate )
                                  .count();

    if ( millisElapsed > PROGRESS_FREQ_MILLIS || chunkBytes == -1 )
    {
        std::lock_guard<std::mutex> transferLock( *m_transfer->mutex );

        if ( m_transfer->status == OpStatus::TRANSFERRING
            || m_transfer->status == OpStatus::PAUSED )
        {
            onUpdate();
            m_transfer->intern.lastProgressUpdate = currentTime;
        }
    }
}

bool FileSender::isFileExecutable( const std::string& chunk,
    const std::wstring& relativePath )
{
    for ( const std::wstring& ext : EXECUTABLE_EXTENSIONS )
    {
        if ( relativePath.rfind( ext ) == relativePath.size() - ext.size() )
        {
            return true;
        }
    }

    return UnixPermissions::checkElfHeader( chunk.data(), chunk.size() );
}

void FileSender::waitIfPaused()
{
    std::unique_lock<std::mutex> lck( m_transfer->intern.pauseLock->mutex );
    if ( m_transfer->intern.pauseLock->value )
    {
        m_transfer->intern.pauseLock->condVar.wait( lck );
        lck.unlock();
    }
}

};
