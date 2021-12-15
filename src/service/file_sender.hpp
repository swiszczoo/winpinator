#pragma once
#include "transfer_types.hpp"

#include "../proto-gen/warp.grpc.pb.h"
#include "../proto-gen/warp.pb.h"

namespace srv
{

class FileSender
{
public:
    explicit FileSender( std::shared_ptr<TransferOp> transfer,
        grpc::ServerWriter<FileChunk>* writer );
    void setCompressionLevel( int level );
    int getCompressionLevel() const;

    bool transferFiles();

private:
    std::shared_ptr<TransferOp> m_transfer;
    grpc::ServerWriter<FileChunk>* m_writer;
    int m_compressLvl;

    bool sendSingleEntity( const std::wstring& root,
        const std::wstring& relativePath );
    bool sendSingleFile( const std::wstring& root,
        const std::wstring& relativePath );
    bool sendSingleDirectory( const std::wstring& root,
        const std::wstring& relativePath );

    bool checkOpFailed();
};

};
