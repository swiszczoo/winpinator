#include "running_instance_detector.hpp"

#include <wx/filename.h>
#include <wx/stdpaths.h>

#include <Windows.h>

RunningInstanceDetector::RunningInstanceDetector( 
    const std::string& lockFileName )
    : m_handle( nullptr )
    , m_anotherRunning( false )
{
    wxLogNull errorLock;

    wxFileName fname( wxStandardPaths::Get().GetUserDataDir(), lockFileName ); 

    m_handle = std::make_unique<wxFile>();
    
    if ( !m_handle->Open( fname.GetFullPath(), wxFile::write_excl ) )
    {
        // We know that another instance is blocking access to the lockfile
        // or that it contains old instance PID number

        m_handle = std::make_unique<wxFile>();

        if ( m_handle->Open( fname.GetFullPath(), wxFile::read ) )
        {
            pid_type pid;
            m_handle->Read( &pid, sizeof( pid_type ) );

            if ( checkPidHasOurImageName( pid ) )
            {
                // We ensured that our second instance is running under that pid
                
                m_anotherRunning = true;
            }
            else
            {
                // This is an old file, just ignore it

                m_handle->Close();
                wxRemoveFile( fname.GetFullPath() );

                m_handle = std::make_unique<wxFile>();
                m_handle->Open( fname.GetFullPath(), wxFile::write_excl );

                if ( !writeCurrentPidToFile() )
                {
                    m_anotherRunning = true;
                }
            }
        }
        else
        {
            wxLogDebug( "Failed to gain read access to lockfile" );
            m_anotherRunning = true;
        }
        
    }
    else
    {
        writeCurrentPidToFile();
    }
}

bool RunningInstanceDetector::isAnotherInstanceRunning()
{
    return m_anotherRunning;
}

void RunningInstanceDetector::free()
{
    m_handle->Close();
    m_handle = nullptr;
}

bool RunningInstanceDetector::checkPidHasOurImageName( pid_type pid )
{
    HANDLE process = OpenProcess( PROCESS_QUERY_LIMITED_INFORMATION,
        FALSE, pid );

    if ( !process )
    {
        return false;
    }

    wchar_t buffer[2048];
    DWORD size = 2048;
    QueryFullProcessImageNameW( process, NULL, buffer, &size );

    wxString processPath( buffer, size );
    wxFileName processName( processPath );

    wxString currentPath = wxStandardPaths::Get().GetExecutablePath();
    wxFileName currentName( currentPath );

    bool result = ( processName.GetFullName().Lower()
        == currentName.GetFullName().Lower() );

    CloseHandle( process );

    return result;
}

bool RunningInstanceDetector::writeCurrentPidToFile()
{
    if ( !m_handle->IsOpened() )
    {
        return false;
    }

    pid_type pid = wxGetProcessId();
    return m_handle->Write( &pid, sizeof( pid_type ) ) == sizeof( pid_type );
}
