#include "zlib_deflate.hpp"

#include <algorithm>

namespace srv
{

const int ZlibDeflate::ZLIB_MAX_COMPRESSION_FACTOR = 1032;

ZlibDeflate::ZlibDeflate( int maxChunkSize )
    : m_maxChunk( maxChunkSize )
{
}

std::string ZlibDeflate::compress( const std::string& input,
    int compressionLevel )
{
    std::string compressed;
    z_stream stream = { 0 };

    setupZlibStream( stream );

    stream.avail_in = input.size();
    stream.next_in = (Bytef*)input.data();

    stream.avail_out = 0;
    stream.next_out = Z_NULL;

    if ( deflateInit( &stream, compressionLevel ) != Z_OK )
    {
        return "";
    }

    uint32_t bufferSize = deflateBound( &stream, input.size() );
    compressed.resize( bufferSize );

    stream.avail_out = bufferSize;
    stream.next_out = (Bytef*)compressed.data();

    deflate( &stream, Z_FINISH );
    uint32_t actualLength = bufferSize - stream.avail_out;

    compressed.resize( actualLength );
    compressed.shrink_to_fit();

    deflateEnd( &stream );
    
    return compressed;
}

std::string ZlibDeflate::decompress( const std::string& input )
{
    std::string decompressed;
    z_stream stream = { 0 };

    setupZlibStream( stream );

    stream.avail_in = input.size();
    stream.next_in = (Bytef*)input.data();

    stream.avail_out = 0;
    stream.next_out = Z_NULL;

    if ( inflateInit( &stream ) != Z_OK )
    {
        return "";
    }

    uint32_t bufferSize = std::min( (unsigned long long)m_maxChunk, 
        (unsigned long long)input.size() * ZLIB_MAX_COMPRESSION_FACTOR );
    decompressed.resize( bufferSize );

    stream.avail_out = bufferSize;
    stream.next_out = (Bytef*)decompressed.data();

    inflate( &stream, Z_NO_FLUSH );
    uint32_t actualLength = bufferSize - stream.avail_out;

    decompressed.resize( actualLength );
    decompressed.shrink_to_fit();

    inflateEnd( &stream );

    return decompressed;
}

void ZlibDeflate::setupZlibStream( z_stream& stream )
{
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
}

};
