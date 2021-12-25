#pragma once
#include "database_types.hpp"

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
    bool clearAllTransfers();
    bool clearAllTransfersForRemote( std::string remoteId );
    bool deleteTransfer( int id );
    std::vector<db::Transfer> queryTransfers( bool queryPaths,
        const std::wstring targetId, const std::string conditions = "" );
    db::Transfer getTransfer( int id, 
        const std::wstring targetId, bool queryPaths );

    bool updateTarget( const db::TargetInfo& target );
    std::vector<db::TargetInfoData> queryTargets();
    bool removeTarget( const std::wstring& targetId );
    bool removeAllTargets();

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

    void queryTransferPaths( db::Transfer& record );
    static inline void fixEnum( int& val, const int unkVal );

    // Update functions
    void performUpdate( int currentLevel );

    bool updateFromVer0ToVer1();
    bool updateFromVer1ToVer2();
    bool updateFromVer2ToVer3();
};

};
