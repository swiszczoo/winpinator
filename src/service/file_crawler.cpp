#include "file_crawler.hpp"

#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/wx.h>

#include <algorithm>
#include <functional>

#include "../thread_name.hpp"

namespace srv
{

const int FileCrawler::MAX_RECURSION_DEPTH = 256;

FileCrawler::FileCrawler()
    : m_lastJobId( 0 )
    , m_sendHidden( true )
{
}

FileCrawler::~FileCrawler()
{
    std::unique_lock<std::mutex> lock( m_mtx );

    for ( auto& pair : m_jobs )
    {
        if ( pair.second.joinable() )
        {
            lock.unlock();
            pair.second.join();
            lock.lock();
        }
    }

    m_jobs.clear();
}

void FileCrawler::setSendHiddenFiles( bool sendHidden )
{
    std::lock_guard<std::mutex> lock( m_mtx );

    m_sendHidden = sendHidden;
}

bool FileCrawler::isSendingHiddenFiles()
{
    std::lock_guard<std::mutex> lock( m_mtx );

    return m_sendHidden;
}

int FileCrawler::startCrawlJob( const std::vector<std::wstring>& paths )
{
    std::lock_guard<std::mutex> lock( m_mtx );

    assert( paths.size() > 0 );

    m_lastJobId++;
    m_jobs[m_lastJobId] = std::thread( std::bind(
        &FileCrawler::crawlJobMain, this, paths ) );

    return m_lastJobId;
}

void FileCrawler::releaseCrawlJob( int jobId )
{
    std::unique_lock<std::mutex> lock( m_mtx );

    if ( m_jobs.find( jobId ) != m_jobs.end() )
    {
        std::thread& thread = m_jobs[jobId];
        lock.unlock();

        if ( thread.joinable() )
        {
            thread.join();
        }

        m_jobs.erase( jobId );
    }
}

void FileCrawler::crawlJobMain( std::vector<std::wstring> paths )
{
    setThreadName( "Crawl job" );

    bool sendHidden;
    {
        std::lock_guard<std::mutex> lock( m_mtx );
        sendHidden = m_sendHidden;
    }

    try
    {
        std::wstring root = findRoot( paths );
        if ( root.empty() )
        {
            throw std::runtime_error( "Returned root path is empty." );
        }

        std::sort( paths.begin(), paths.end() );

        auto pathVec = std::make_shared<std::vector<std::wstring>>();
        wxFileName location = wxFileName::DirName( root );
        wxFileName relativeLoc;

        for ( auto& path : paths )
        {
            findAndUnwindElement( wxFileName( path ),
                location, relativeLoc, pathVec.get(), sendHidden );
        }
    }
    catch ( std::exception& e )
    {
        wxLogDebug( "FileCrawler: failed! reason: %s", e.what() );

        // Handle failed crawl job
    }
}

std::wstring FileCrawler::findRoot( const std::vector<std::wstring>& paths )
{
    wxString firstPath = paths[0];
    wxFileName firstName = pathToFileName( firstPath );

    wxString volume = firstName.GetVolume();
    wxArrayString segments = firstName.GetDirs();
    bool volumeOk = true;

    std::vector<wxFileName> fileNameCache;
    fileNameCache.reserve( paths.size() );

    for ( int i = 0; i < paths.size(); i++ )
    {
        wxFileName fname = pathToFileName( paths[i] );
        fileNameCache.push_back( fname );

        if ( fname.GetVolume() != volume )
        {
            volumeOk = false;
            break;
        }
    }

    if ( !volumeOk )
    {
        return L"";
    }

    wxString commonDirs = wxEmptyString;

    for ( int i = 0; i < segments.GetCount(); i++ )
    {
        wxString segment = segments[i];
        bool iterOk = true;

        for ( int j = 1; j < paths.size(); j++ )
        {
            wxFileName& fname = fileNameCache[j];
            if ( fname.GetDirCount() <= i || fname.GetDirs()[i] != segment )
            {
                iterOk = false;
                break;
            }
        }

        if ( iterOk )
        {
            commonDirs += '/';
            commonDirs += segment;
        }
        else
        {
            break;
        }
    }

    return wxFileName( volume, commonDirs, "", "" ).GetFullPath().ToStdWstring();
}

wxFileName FileCrawler::pathToFileName( const wxString& path )
{
    if ( wxDirExists( path ) )
    {
        return wxFileName::DirName( path );
    }
    else
    {
        return wxFileName( path );
    }
}

void FileCrawler::findAndUnwindElement( wxFileName elementLoc,
    wxFileName& currentLocation, wxFileName& relativeLoc,
    std::vector<std::wstring>* paths, bool sendHidden )
{
    int lastOk = -1;
    for ( int i = 0; i < currentLocation.GetDirCount(); i++ )
    {
        if ( elementLoc.GetDirCount() > i
            && elementLoc.GetDirs()[i] == currentLocation.GetDirs()[i] )
        {
            lastOk = i;
        }
        else
        {
            break;
        }
    }
    lastOk++;

    while ( currentLocation.GetDirCount() > lastOk )
    {
        currentLocation.RemoveLastDir();
        relativeLoc.RemoveLastDir();
    }

    for ( int i = currentLocation.GetDirCount(); i < elementLoc.GetDirCount(); i++ )
    {
        const wxString& dir = elementLoc.GetDirs()[i];
        currentLocation.AppendDir( dir );
        relativeLoc.AppendDir( dir );

        paths->push_back( relativeLoc.GetFullPath().RemoveLast().ToStdWstring() );
    }

    relativeLoc.AppendDir( elementLoc.GetFullName() );

    performFilesystemDFS( elementLoc, relativeLoc, paths, sendHidden );
}

void FileCrawler::performFilesystemDFS( wxFileName location, 
    wxFileName relativeLoc, std::vector<std::wstring>* paths, 
    bool sendHidden, int recursionLevel )
{
    if ( recursionLevel > MAX_RECURSION_DEPTH )
    {
        throw std::runtime_error( "max recursion depth exceeded!" );
    }

    if ( location.DirExists() )
    {
        paths->push_back( 
            relativeLoc.GetFullPath().RemoveLast().ToStdWstring() );

        int flags = wxDIR_FILES | wxDIR_DIRS | ( sendHidden ? wxDIR_HIDDEN : 0 );

        wxArrayString files;
        wxDir::GetAllFiles( location.GetFullPath(), &files, wxEmptyString, flags );

        for ( auto string : files )
        {
            wxFileName newLocation = wxFileName::DirName( string );
            wxFileName newRelativeLoc = relativeLoc;

            newRelativeLoc.AppendDir( 
                newLocation.GetDirs()[newLocation.GetDirCount() - 1] );

            performFilesystemDFS( newLocation, newRelativeLoc, 
                paths, sendHidden, recursionLevel + 1 );
        }
    }
    else if ( wxFileExists( location.GetFullPath().RemoveLast() ) )
    {
        paths->push_back(
            relativeLoc.GetFullPath().RemoveLast().ToStdWstring() );
    }
}

};
