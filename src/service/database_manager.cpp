#include "database_manager.hpp"

#include <algorithm>

#define UPD_EXEC( command ) results |= sqlite3_exec( m_db, command, NULL, NULL, NULL )
#define FIX_ENUM( val, unk ) DatabaseManager::fixEnum( (int&)( val ), (int)( unk ) )
#define FIX_RESULTS( res )    \
    if ( res == SQLITE_DONE ) \
    res = SQLITE_OK

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
        // A unusual error occurred. Maybe the file is corrupt?
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
        sqlite3_exec( m_db, "VACUUM;", NULL, NULL, NULL );
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
            "SELECT value FROM meta WHERE key='db_version';", -1, &qry, NULL );

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

void DatabaseManager::beginTransaction()
{
    sqlite3_exec( m_db, "BEGIN TRANSACTION;", NULL, NULL, NULL );
}

bool DatabaseManager::endTransaction( int result )
{
    if ( result == SQLITE_OK )
    {
        sqlite3_exec( m_db, "COMMIT;", NULL, NULL, NULL );
        return true;
    }
    else
    {
        sqlite3_exec( m_db, "ROLLBACK;", NULL, NULL, NULL );
        return false;
    }
}

inline void DatabaseManager::fixEnum( int& val, const int unkVal )
{
    if ( val < 0 || val > unkVal )
    {
        val = unkVal;
    }
}

void DatabaseManager::performUpdate( int currentLevel )
{
    beginTransaction();

    bool updateSuccess = false;
    if ( currentLevel >= 0 && currentLevel < m_updFunctions.size() )
    {
        updateSuccess = m_updFunctions[currentLevel]();
    }

    if ( updateSuccess )
    {
        sqlite3_exec( m_db, "COMMIT;", NULL, NULL, NULL );
    }
    else
    {
        sqlite3_exec( m_db, "ROLLBACK;", NULL, NULL, NULL );
    }
}

bool DatabaseManager::updateFromVer0ToVer1()
{
    // This routine creates entire db structure from scratch

    int results = 0;

    UPD_EXEC( "CREATE TABLE IF NOT EXISTS meta( "
              "  [key] TEXT PRIMARY KEY UNIQUE, "
              "  value "
              "); " );
    UPD_EXEC( "CREATE TABLE IF NOT EXISTS transfers( "
              "  [id] INTEGER PRIMARY KEY AUTOINCREMENT, "
              "  target_id TEXT, "
              "  single_name TEXT, "
              "  transfer_type INTEGER, "
              "  transfer_timestamp INTEGER, "
              "  file_count INTEGER, "
              "  folder_count INTEGER, "
              "  total_size_bytes INTEGER, "
              "  outgoing BOOLEAN, "
              "  status INTEGER "
              "); " );
    UPD_EXEC( "CREATE INDEX idx_transfer_target ON transfers( target_id );" );
    UPD_EXEC( "CREATE TABLE IF NOT EXISTS transfer_paths("
              "  [id] INTEGER PRIMARY KEY AUTOINCREMENT, "
              "  transfer_id INTEGER, "
              "  element_name TEXT, "
              "  element_type INTEGER, "
              "  relative_path TEXT, "
              "  absolute_path TEXT "
              ");" );
    UPD_EXEC( "CREATE INDEX idx_path_foreign_key "
              "ON transfer_paths( transfer_id );" );
    UPD_EXEC( "CREATE VIEW dbg_transfers AS "
              "SELECT transfers.*, transfer_paths.relative_path AS relative "
              "FROM transfers LEFT OUTER JOIN transfer_paths "
              "ON transfer_paths.transfer_id = transfers.id " );
    UPD_EXEC( "INSERT INTO meta VALUES ( 'db_version', 1 );" );

    return results == SQLITE_OK;
}

// Data query and modification functions

bool DatabaseManager::addTransfer( const db::Transfer& record )
{
    std::lock_guard<std::mutex> guard( m_mutex );

    if ( !m_dbOpen )
    {
        return false;
    }

    int results = 0;

    beginTransaction();

    // Insert the 'transfer' record

    sqlite3_stmt* transferStmt;
    sqlite3_prepare_v2( m_db,
        "INSERT INTO transfers( target_id, single_name, transfer_type, "
        "transfer_timestamp, file_count, folder_count, total_size_bytes, "
        "outgoing, status ) "
        "VALUES ( ?, ?, ?, ?, ?, ?, ?, ?, ? );",
        -1, &transferStmt, NULL );

    sqlite3_bind_text16( transferStmt, 1,
        record.targetId.c_str(), -1, SQLITE_STATIC );
    sqlite3_bind_text16( transferStmt, 2,
        record.singleElementName.c_str(), -1, SQLITE_STATIC );
    sqlite3_bind_int( transferStmt, 3, (int)record.transferType );
    sqlite3_bind_int64( transferStmt, 4,
        (sqlite3_int64)record.transferTimestamp );
    sqlite3_bind_int( transferStmt, 5, record.fileCount );
    sqlite3_bind_int( transferStmt, 6, record.folderCount );
    sqlite3_bind_int64( transferStmt, 7, (sqlite3_int64)record.totalSizeBytes );
    sqlite3_bind_int( transferStmt, 8, record.outgoing ? 1 : 0 );
    sqlite3_bind_int( transferStmt, 9, (int)record.status );

    results |= sqlite3_step( transferStmt );
    FIX_RESULTS( results );

    int transferId = sqlite3_last_insert_rowid( m_db );

    sqlite3_finalize( transferStmt );

    // Insert 'transfer_path' records

    sqlite3_stmt* pathStmt;
    sqlite3_prepare_v2( m_db,
        "INSERT INTO transfer_paths( transfer_id, element_name, "
        "element_type, relative_path, absolute_path ) "
        "VALUES ( ?, ?, ?, ?, ? );",
        -1, &pathStmt, NULL );

    for ( const db::TransferElement& element : record.elements )
    {
        sqlite3_reset( pathStmt );
        sqlite3_clear_bindings( pathStmt );

        sqlite3_bind_int( pathStmt, 1, transferId );
        sqlite3_bind_text16( pathStmt, 2,
            element.elementName.c_str(), -1, SQLITE_STATIC );
        sqlite3_bind_int( pathStmt, 3, (int)element.elementType );
        sqlite3_bind_text16( pathStmt, 4,
            element.relativePath.c_str(), -1, SQLITE_STATIC );
        sqlite3_bind_text16( pathStmt, 5,
            element.absolutePath.c_str(), -1, SQLITE_STATIC );

        results |= sqlite3_step( pathStmt );
        FIX_RESULTS( results );
    }

    sqlite3_finalize( pathStmt );

    return endTransaction( results );
}

bool DatabaseManager::clearAllTransfers()
{
    std::lock_guard<std::mutex> guard( m_mutex );

    if ( !m_dbOpen )
    {
        return false;
    }

    int results = 0;

    beginTransaction();
    results |= sqlite3_exec( m_db,
        "DELETE FROM transfers;", NULL, NULL, NULL );
    FIX_RESULTS( results );
    results |= sqlite3_exec( m_db,
        "DELETE FROM transfer_paths;", NULL, NULL, NULL );
    FIX_RESULTS( results );
    return endTransaction( results );
}

bool DatabaseManager::clearAllTransfersForRemote( std::string remoteId )
{
    std::lock_guard<std::mutex> guard( m_mutex );

    if ( !m_dbOpen )
    {
        return false;
    }

    int results = 0;

    beginTransaction();

    sqlite3_stmt* pathsStmt;
    sqlite3_prepare_v2( m_db,
        "DELETE FROM transfer_paths WHERE ROWID IN ( "
        "  SELECT transfer_paths.ROWID FROM transfers "
        "  INNER JOIN transfer_paths ON ( "
        "    transfers.id = transfer_paths.transfer_id "
        "  ) "
        "  WHERE transfers.target_id = ?"
        ");",
        -1, &pathsStmt, NULL );
    sqlite3_bind_text( pathsStmt, 1, remoteId.c_str(), -1, SQLITE_STATIC );
    results |= sqlite3_step( pathsStmt );
    sqlite3_finalize( pathsStmt );
    FIX_RESULTS( results );

    sqlite3_stmt* transferStmt;
    sqlite3_prepare_v2( m_db,
        "DELETE FROM transfers WHERE target_id=?",
        -1, &transferStmt, NULL );
    sqlite3_bind_text( transferStmt, 1, remoteId.c_str(), -1, SQLITE_STATIC );
    results |= sqlite3_step( transferStmt );
    sqlite3_finalize( transferStmt );
    FIX_RESULTS( results );

    return endTransaction( results );
}

bool DatabaseManager::deleteTransfer( int id )
{
    std::lock_guard<std::mutex> guard( m_mutex );

    if ( !m_dbOpen )
    {
        return false;
    }

    int results = 0;

    beginTransaction();

    sqlite3_stmt* deleteStmt;
    sqlite3_prepare_v2( m_db, "DELETE FROM transfers WHERE id=?;",
        -1, &deleteStmt, NULL );
    sqlite3_bind_int( deleteStmt, 1, id );
    results |= sqlite3_step( deleteStmt );
    FIX_RESULTS( results );
    sqlite3_finalize( deleteStmt );

    sqlite3_stmt* pathsStmt;
    sqlite3_prepare_v2( m_db, "DELETE FROM transfer_paths WHERE transfer_id=?;",
        -1, &pathsStmt, NULL );
    sqlite3_bind_int( pathsStmt, 1, id );
    results |= sqlite3_step( pathsStmt );
    FIX_RESULTS( results );
    sqlite3_finalize( pathsStmt );

    return endTransaction( results );
}

std::vector<db::Transfer> DatabaseManager::queryTransfers( bool queryPaths,
    const std::wstring targetId, const std::string conditions )
{
    std::lock_guard<std::mutex> guard( m_mutex );

    if ( !m_dbOpen )
    {
        return {};
    }

    std::vector<db::Transfer> records;

    sqlite3_stmt* queryStmt;

    if ( conditions.empty() )
    {
        sqlite3_prepare_v2( m_db,
            "SELECT * FROM transfers WHERE target_id=? "
            "ORDER BY transfer_timestamp DESC;",
            -1, &queryStmt, NULL );
    }
    else
    {
        std::string query = "SELECT * FROM transfers WHERE (";
        query += conditions;
        query += ") AND target_id=? ORDER BY transfer_timestamp DESC;";

        sqlite3_prepare_v2( m_db, query.c_str(), -1, &queryStmt, NULL );
    }

    sqlite3_bind_text16( queryStmt, 1, targetId.c_str(), -1, SQLITE_STATIC );

    while ( sqlite3_step( queryStmt ) != SQLITE_DONE )
    {
        db::Transfer record;

        record.id
            = sqlite3_column_int( queryStmt, 0 );
        record.targetId
            = (const wchar_t*)sqlite3_column_text16( queryStmt, 1 );
        record.singleElementName
            = (const wchar_t*)sqlite3_column_text16( queryStmt, 2 );
        record.transferType
            = (db::TransferType)sqlite3_column_int( queryStmt, 3 );
        record.transferTimestamp
            = sqlite3_column_int64( queryStmt, 4 );
        record.fileCount
            = sqlite3_column_int( queryStmt, 5 );
        record.folderCount
            = sqlite3_column_int( queryStmt, 6 );
        record.totalSizeBytes
            = sqlite3_column_int64( queryStmt, 7 );
        record.outgoing
            = (bool)sqlite3_column_int( queryStmt, 8 );
        record.status
            = (db::TransferStatus)sqlite3_column_int( queryStmt, 9 );

        FIX_ENUM( record.transferType, db::TransferType::UNKNOWN );
        FIX_ENUM( record.status, db::TransferStatus::UNKNOWN );

        if ( queryPaths )
        {
            queryTransferPaths( record );
        }

        records.push_back( record );
    }

    sqlite3_finalize( queryStmt );

    return records;
}

void DatabaseManager::queryTransferPaths( db::Transfer& record )
{
    sqlite3_stmt* pathStmt;
    sqlite3_prepare_v2( m_db, "SELECT * FROM transfer_paths WHERE transfer_id=?;",
        -1, &pathStmt, NULL );
    sqlite3_bind_int( pathStmt, 1, record.id );

    while ( sqlite3_step( pathStmt ) != SQLITE_DONE )
    {
        db::TransferElement pathRecord;
        pathRecord.id
            = sqlite3_column_int( pathStmt, 0 );
        pathRecord.elementName
            = (const wchar_t*)sqlite3_column_text16( pathStmt, 2 );
        pathRecord.elementType
            = (db::TransferElementType)sqlite3_column_int( pathStmt, 3 );
        pathRecord.relativePath
            = (const wchar_t*)sqlite3_column_text16( pathStmt, 4 );
        pathRecord.absolutePath
            = (const wchar_t*)sqlite3_column_text16( pathStmt, 5 );

        FIX_ENUM( pathRecord.elementType, db::TransferElementType::UNKNOWN );

        record.elements.push_back( pathRecord );
    }

    sqlite3_finalize( pathStmt );
}

db::Transfer DatabaseManager::getTransfer( int id,
    const std::wstring targetId, bool queryPaths )
{
    std::string condition = "id=" + std::to_string( id );

    auto result = queryTransfers( queryPaths, targetId, condition );

    if ( result.empty() )
    {
        db::Transfer transfer;
        transfer.id = -1;
        return transfer;
    }

    return result[0];
}

};
