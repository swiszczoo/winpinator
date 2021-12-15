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

FileSender::FileSender( std::shared_ptr<TransferOp> transfer,
    grpc::ServerWriter<FileChunk>* writer )
    : m_transfer( transfer )
    , m_writer( writer )
    , m_compressLvl( 0 )
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

bool FileSender::transferFiles()
{
    int size = m_transfer->intern.relativePaths->size();
    std::wstring root = m_transfer->intern.rootDir;

    for ( int i = 0; i < size; i++ )
    {
        if ( checkOpFailed() )
        {
            return false;
        }

        if ( !sendSingleEntity( root, m_transfer->intern.relativePaths->at( i ) ) )
        {
            return false;
        }
    }

    return true;
}

bool FileSender::sendSingleEntity( const std::wstring& root,
    const std::wstring& relativePath )
{
    wxString fullPath = root + "\\" + relativePath;

    if ( wxFileExists( fullPath ) )
    {
        return sendSingleFile( root, relativePath );
    }
    else if ( wxDirExists( fullPath ) )
    {
        return sendSingleDirectory( root, relativePath );
    }

    return false;
}

bool FileSender::sendSingleFile( const std::wstring& root,
    const std::wstring& relativePath )
{
    wxLogDebug( "FileSender: Sending file %s", wxString( relativePath ) );

    WXLOGNULL;
    return true;
}

bool FileSender::sendSingleDirectory(const std::wstring& root,
    const std::wstring& relativePath)
{
    wxLogDebug( "FileSender: Sending directory %s", wxString( relativePath ) );

    WXLOGNULL;
    return true;
}

bool FileSender::checkOpFailed()
{
    std::lock_guard<std::mutex> guard( *m_transfer->mutex );

    return m_transfer->status == OpStatus::FAILED
        || m_transfer->status == OpStatus::FAILED_UNRECOVERABLE;
}

};
