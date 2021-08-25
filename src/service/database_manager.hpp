#pragma once
#include <wx/wx.h>

#include <sqlite3.h>

#include <mutex>

namespace srv
{

class DatabaseManager
{
public:
    explicit DatabaseManager( const wxString& dbFile );
    ~DatabaseManager();

private:
    static const int TARGET_DB_VER;

    std::mutex m_mutex;

    sqlite3* m_db;
    bool m_dbOpen;

    bool enforceIntegrity();

    // Update functions
    void performUpdate( int currentLevel );
    void updateFromVer0ToVer1();
};

};
