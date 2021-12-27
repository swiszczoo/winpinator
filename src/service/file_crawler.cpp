#include "file_crawler.hpp"

#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/wx.h>

#include "../globals.hpp"

#include <algorithm>
#include <functional>

#include "../thread_name.hpp"

namespace srv
{

const int FileCrawler::MAX_RECURSION_DEPTH = 256;
const long long FileCrawler::BLOCK_SIZE = 4096LL;

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
        &FileCrawler::crawlJobMain, this, paths, m_lastJobId ) );

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

void FileCrawler::crawlJobMain( std::vector<std::wstring> paths, int jobId )
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

        std::set<std::wstring> pathSet;
        wxFileName location = wxFileName::DirName( root );
        wxFileName relativeLoc;
        long long totalSize = 0;

        for ( auto& path : paths )
        {
            if ( path[path.size() - 1] == '\\' )
            {
                findAndUnwindElement( 
                    wxFileName( path.substr( 0, path.size() - 1 ) ),
                    location, relativeLoc, &pathSet, sendHidden, totalSize );
            }
            else
            {
                findAndUnwindElement( wxFileName( path ),
                    location, relativeLoc, &pathSet, sendHidden, totalSize );
            }
        }

        auto pathVec = std::make_shared<std::vector<std::wstring>>();
        pathVec->reserve( pathSet.size() );
        for ( auto& path : pathSet )
        {
            pathVec->push_back( path );
        }

        srv::Event successEvent;
        successEvent.type = srv::EventType::OUTCOMING_CRAWLER_SUCCEEDED;
        successEvent.eventData.crawlerOutputData 
            = std::make_shared<srv::CrawlerOutputData>();
        successEvent.eventData.crawlerOutputData->jobId = jobId; 
        successEvent.eventData.crawlerOutputData->rootDir = root;
        successEvent.eventData.crawlerOutputData->paths = pathVec;
        successEvent.eventData.crawlerOutputData->totalSize = totalSize;

        // Analyze top dir basenames
        std::set<wxString> topDirSet;
        for ( const auto& path : *pathVec )
        {
            wxString pathString( path );
            topDirSet.insert( pathString.BeforeFirst( L'\\' ) );
        }

        int folderCount = 0;
        int fileCount = 0;
        for ( const auto& path : topDirSet )
        {
            successEvent.eventData.crawlerOutputData
                ->topDirBasenamesUtf8.push_back( std::string( path.ToUTF8() ) );

            wxFileName pathName = wxFileName::DirName( root );
            pathName.SetFullName( path );

            if ( wxFileExists( pathName.GetFullPath() ) )
            {
                fileCount++;
            }
            else if ( wxDirExists( pathName.GetFullPath() ) )
            {
                folderCount++;
            }
        }

        successEvent.eventData.crawlerOutputData->fileCount = fileCount;
        successEvent.eventData.crawlerOutputData->folderCount = folderCount;
        
        Globals::get()->getWinpinatorServiceInstance()->postEvent( successEvent );
    }
    catch ( std::exception& e )
    {
        wxLogDebug( "FileCrawler: failed! reason: %s", e.what() );

        srv::Event failEvent;
        failEvent.type = srv::EventType::OUTCOMING_CRAWLER_FAILED;
        failEvent.eventData.crawlerFailJobId = jobId;

        Globals::get()->getWinpinatorServiceInstance()->postEvent( failEvent );
    }
}

std::wstring FileCrawler::findRoot( const std::vector<std::wstring>& paths )
{
    wxString firstPath = paths[0];
    wxFileName firstName( firstPath );

    wxString volume = firstName.GetVolume();
    wxArrayString segments = firstName.GetDirs();
    bool volumeOk = true;

    std::vector<wxFileName> fileNameCache;
    fileNameCache.reserve( paths.size() );

    for ( int i = 0; i < paths.size(); i++ )
    {
        wxFileName fname( paths[i] );
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
    std::set<std::wstring>* paths, bool sendHidden,
    long long& totalSize )
{
    wxLogDebug( "FileCrawler: cur: %s rel: %s", 
        currentLocation.GetFullPath(), relativeLoc.GetFullPath() );

    int lastOk = -1;
    for ( int i = 0; i < currentLocation.GetDirCount(); i++ )
    {
        if ( elementLoc.GetDirCount() > i
            && elementLoc.GetDirs()[i] == currentLocation.GetDirs()[i] )
        {
            wxLogDebug( "FileCrawler: folder ok %s", 
                currentLocation.GetDirs()[i] );
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

        paths->insert( relativeLoc.GetFullPath().RemoveLast().ToStdWstring() );
    }

    currentLocation.AppendDir( elementLoc.GetFullName() );
    relativeLoc.AppendDir( elementLoc.GetFullName() );

    performFilesystemDFS( wxFileName::DirName( elementLoc.GetFullPath() ), 
        relativeLoc, paths, sendHidden, totalSize );
}

void FileCrawler::performFilesystemDFS( wxFileName location, 
    wxFileName relativeLoc, std::set<std::wstring>* paths, 
    bool sendHidden, long long& totalSize, int recursionLevel )
{
#ifndef _DEBUG
    wxLogNull logNull;
#endif

    wxLogDebug( "FileCrawler: DFS | loc %s, rel %s",
        location.GetFullPath(), relativeLoc.GetFullPath() );

    if ( recursionLevel > MAX_RECURSION_DEPTH )
    {
        throw std::runtime_error( "max recursion depth exceeded!" );
    }

    if ( location.DirExists() )
    {
        paths->insert( 
            relativeLoc.GetFullPath().RemoveLast().ToStdWstring() );

        totalSize += BLOCK_SIZE;

        int flags = wxDIR_FILES | wxDIR_DIRS | ( sendHidden ? wxDIR_HIDDEN : 0 );

        wxArrayString files;
        wxDir dir( location.GetFullPath() );
        wxString currPath;

        for ( bool cont = dir.GetFirst( &currPath, wxEmptyString, flags ); 
            cont; cont = dir.GetNext( &currPath ) )
        {
            files.Add( currPath );
        }

        for ( auto string : files )
        {
            wxFileName newLocation = location;
            newLocation.AppendDir( string );

            wxFileName newRelativeLoc = relativeLoc;
            newRelativeLoc.AppendDir( 
                newLocation.GetDirs()[newLocation.GetDirCount() - 1] );

            performFilesystemDFS( newLocation, newRelativeLoc, 
                paths, sendHidden, totalSize, recursionLevel + 1 );
        }
    }
    else if ( wxFileExists( location.GetFullPath().RemoveLast() ) )
    {
        paths->insert(
            relativeLoc.GetFullPath().RemoveLast().ToStdWstring() );

        wxFileName fname( location.GetFullPath().RemoveLast() );
        totalSize += ceil( 
            (long double)fname.GetSize().GetValue() / BLOCK_SIZE ) * BLOCK_SIZE;
    }
}

};
