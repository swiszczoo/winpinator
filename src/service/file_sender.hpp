#pragma once
#include "transfer_types.hpp"
#include "unix_permissions.hpp"
#include "zlib_deflate.hpp"

#include "../proto-gen/warp.grpc.pb.h"
#include "../proto-gen/warp.pb.h"

#include <functional>
#include <vector>

namespace srv
{

class FileSender
{
public:
    explicit FileSender( std::shared_ptr<TransferOp> transfer,
        grpc::ServerWriter<FileChunk>* writer );
    void setCompressionLevel( int level );
    int getCompressionLevel() const;

    void setUnixPermissionMasks( int file, int executable, int directory );
    int getUnixFilePermissionMask();
    int getUnixExecutablePermissionMask();
    int getUnixDirectoryPermissionMask();

    bool transferFiles( std::function<void()> onUpdate );

private:
    static const long long PROGRESS_FREQ_MILLIS;
    static const long long FILE_CHUNK_SIZE;
    static const std::vector<std::wstring> EXECUTABLE_EXTENSIONS;

    std::shared_ptr<TransferOp> m_transfer;
    grpc::ServerWriter<FileChunk>* m_writer;
    int m_compressLvl;

    UnixPermissions m_filePerms;
    UnixPermissions m_execPerms;
    UnixPermissions m_dirPerms;

    ZlibDeflate m_compressor;

    bool sendSingleEntity( const std::wstring& root,
        const std::wstring& relativePath, std::function<void()>& onUpdate );
    bool sendSingleFile( const std::wstring& root,
        const std::wstring& relativePath, std::function<void()>& onUpdate );
    bool sendSingleDirectory( const std::wstring& root,
        const std::wstring& relativePath, std::function<void()>& onUpdate );

    bool checkOpFailed();
    void updateProgress( long long chunkBytes, std::function<void()>& onUpdate );
    bool isFileExecutable( const std::string& chunk, 
        const std::wstring& relativePath );
    void waitIfPaused();
};

};
