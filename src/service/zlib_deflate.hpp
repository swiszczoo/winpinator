#pragma once
#include <zlib.h>

#include <string>

namespace srv
{

class ZlibDeflate
{
public:
    explicit ZlibDeflate( int maxChunkSize = 8 * 1024 * 1024 );

    std::string compress( const std::string& input, int compressionLevel );
    std::string decompress( const std::string& input );

private:
    static const int ZLIB_MAX_COMPRESSION_FACTOR;
    int m_maxChunk;

    void setupZlibStream( z_stream& stream );
};

};
