#pragma once
#include "database_types.h"

#include <wx/wx.h>

#include <sqlite3.h>

#include <mutex>
#include <vector>

namespace srv
{

class DatabaseManager
{
public:
    explicit DatabaseManager( const wxString& dbFile );
    ~DatabaseManager();

    bool isDatabaseAvailable();

    // Data query and modification functions

    bool addTransfer( const db::Transfer& record );

private:
    static const int TARGET_DB_VER;

    std::vector<std::function<bool()>> m_updFunctions;
    std::mutex m_mutex;

    sqlite3* m_db;
    bool m_dbOpen;

    void setupUpdateFunctionVector();
    bool enforceIntegrity();

    void beginTransaction();
    bool endTransaction( int result );

    // Update functions
    void performUpdate( int currentLevel );

    bool updateFromVer0ToVer1();
};

};
