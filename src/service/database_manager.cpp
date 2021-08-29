#include "database_manager.hpp"

namespace srv
{

const int DatabaseManager::TARGET_DB_VER = 1;

DatabaseManager::DatabaseManager( const wxString& dbPath )
    : m_db( nullptr )
    , m_dbOpen( false )
{
    int err = sqlite3_open16( dbPath.wc_str(), &m_db );

    if ( err != SQLITE_OK && err != SQLITE_CANTOPEN )
    {
        // A unusual error occured. Maybe the file is corrupt?
        // Try removing db file from disk.

        if ( wxFileExists( dbPath ) )
        {
            wxRemoveFile( dbPath ); // Ignore return value

            int newErr = sqlite3_open16( dbPath.wc_str(), &m_db );

            if ( newErr == SQLITE_OK || newErr == SQLITE_CANTOPEN )
            {
                m_dbOpen = true;
            }
        }
    }
    else
    {
        m_dbOpen = true;
    }

    setupUpdateFunctionVector();
    enforceIntegrity();
}

DatabaseManager::~DatabaseManager()
{
    std::lock_guard<std::mutex> guard( m_mutex );

    m_dbOpen = false;

    if ( m_db )
    {
        sqlite3_close( m_db );
    }
}

bool DatabaseManager::isDatabaseAvailable()
{
    std::lock_guard<std::mutex> guard( m_mutex );

    return m_dbOpen;
}

void DatabaseManager::setupUpdateFunctionVector()
{
    m_updFunctions = {
        std::bind( &DatabaseManager::updateFromVer0ToVer1, this ) // 0 -> 1
    };
}

bool DatabaseManager::enforceIntegrity()
{
    if ( !m_dbOpen )
    {
        return false;
    }

    sqlite3_stmt* qry = nullptr;
    int currentVersion = 0;

    do
    {
        sqlite3_prepare_v2( m_db,
            "SELECT value FROM meta WHERE key='db_version'", -1, &qry, NULL );

        if ( qry )
        {
            int stepResult = sqlite3_step( qry );
            if ( stepResult == SQLITE_ROW )
            {
                currentVersion = sqlite3_column_int( qry, 0 );
            }

            sqlite3_finalize( qry );
        }

        if ( currentVersion < DatabaseManager::TARGET_DB_VER )
        {
            performUpdate( currentVersion );
        }

    } while ( currentVersion < DatabaseManager::TARGET_DB_VER );

    return true;
}

void DatabaseManager::performUpdate( int currentLevel )
{
    sqlite3_exec( m_db, "BEGIN TRANSACTION", NULL, NULL, NULL );

    bool updateSuccess = false;
    if ( currentLevel > 0 && currentLevel < m_updFunctions.size() )
    {
        updateSuccess = m_updFunctions[currentLevel]();
    }

    if ( updateSuccess )
    {
        sqlite3_exec( m_db, "COMMIT", NULL, NULL, NULL );
    }
    else
    {
        sqlite3_exec( m_db, "ROLLBACK", NULL, NULL, NULL );
    }
}

bool DatabaseManager::updateFromVer0ToVer1()
{
    // This routine creates entire db structure from scratch

    int results = 0;

    results |= sqlite3_exec( m_db,
        "CREATE TABLE IF NOT EXISTS meta( "
        "  [key] TEXT PRIMARY KEY UNIQUE, "
        "  value "
        "); ",
        NULL, NULL, NULL );

    results |= sqlite3_exec( m_db, 
        "CREATE TABLE IF NOT EXISTS transfers( "
        "  [id] INTEGER PRIMARY KEY AUTOINCREMENT, "
        "  target_id TEXT, "
        "  transfer_type INTEGER, "
        "  transfer_timestamp INTEGER, "
        "  file_count INTEGER, "
        "  folder_count INTEGER, "
        "  total_size_bytes INTEGER, "
        "  outgoing BOOLEAN, "
        "  status INTEGER "
        "); ",
        NULL, NULL, NULL );

    results |= sqlite3_exec( m_db,
        "CREATE INDEX idx_transfer_target ON transfers( target_id );",
        NULL, NULL, NULL );

    results |= sqlite3_exec( m_db,
        "INSERT INTO meta VALUES ( 'db_version', 1 );", NULL, NULL, NULL );

    return results == SQLITE_OK;
}

};
