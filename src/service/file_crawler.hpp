#pragma once
#include <wx/filename.h>

#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

namespace srv
{

class FileCrawler
{
public:
    FileCrawler();
    ~FileCrawler();

    void setSendHiddenFiles( bool sendHidden );
    bool isSendingHiddenFiles();

    int startCrawlJob( const std::vector<std::wstring>& paths );
    void releaseCrawlJob( int jobId );

private:
    static const int MAX_RECURSION_DEPTH;

    std::map<int, std::thread> m_jobs;
    std::mutex m_mtx;
    int m_lastJobId;
    bool m_sendHidden;

    void crawlJobMain( std::vector<std::wstring> paths );

    std::wstring findRoot( const std::vector<std::wstring>& paths );
    wxFileName pathToFileName( const wxString& path );
    void findAndUnwindElement( wxFileName elementLoc, 
        wxFileName& currentLocation, wxFileName& relativeLoc, 
        std::set<std::wstring>* paths, bool sendHidden );
    void performFilesystemDFS( wxFileName location, wxFileName relativeLoc,
        std::set<std::wstring>* paths, bool sendHidden, 
        int recursionLevel = 0 );
};

};
