#include "account_picture_extractor.hpp"

#include <wx/filename.h>
#include <wx/mstream.h>
#include <wx/msw/registry.h>
#include <wx/utils.h>

#include <memory>

const wxString AccountPictureExtractor::s_keyName = "SOFTWARE\\Microsoft"
                                                    "\\Windows\\CurrentVersion"
                                                    "\\AccountPicture";

AccountPictureExtractor::AccountPictureExtractor()
    : m_error( ExtractorError::OK )
    , m_loRes( wxNullImage )
    , m_hiRes( wxNullImage )
{
}

bool AccountPictureExtractor::process()
{
    // First of all, try to gather avatar filename from
    // Windows registry

    wxRegKey key( wxRegKey::HKCU, AccountPictureExtractor::s_keyName );

    if ( !key.Exists() )
    {
        m_error = ExtractorError::REG_KEY_NOT_FOUND;
        return false;
    }

    if ( !key.Open( wxRegKey::AccessMode::Read ) )
    {
        m_error = ExtractorError::REG_KEY_NOT_FOUND;
        return false;
    }

    wxString sourceBuf;
    key.QueryValue( "SourceId", sourceBuf );

    if ( sourceBuf.IsEmpty() )
    {
        m_error = ExtractorError::REG_KEY_NOT_FOUND;
        return false;
    }

    //
    // Then try to find the appropriate .accountpicture-ms file. It should
    // be stored in AppData\Roaming\Microsoft\Windows\AccountPictures

    wxString appDataDir;
    if ( !wxGetEnv( "APPDATA", &appDataDir ) )
    {
        m_error = ExtractorError::AVATAR_NOT_FOUND;
        return false;
    }

    wxFileName avatarDir( appDataDir, "" );
    avatarDir.AppendDir( "Microsoft" );
    avatarDir.AppendDir( "Windows" );
    avatarDir.AppendDir( "AccountPictures" );
    
    if ( !avatarDir.DirExists() )
    {
        m_error = ExtractorError::AVATAR_NOT_FOUND;
        return false;
    }

    wxFileName avatarFile( avatarDir.GetFullPath(),
        wxString::Format( "%s.accountpicture-ms", sourceBuf ) );

    if ( !avatarFile.Exists() )
    {
        m_error = ExtractorError::AVATAR_NOT_FOUND;
        return false;
    }

    //
    // Open the file and try to find and extract two JPEG images from it

    wxFile avatar( avatarFile.GetFullPath(), wxFile::read );
    if ( !avatar.IsOpened() )
    {
        m_error = ExtractorError::AVATAR_FILE_UNREADABLE;
        return false;
    }

    size_t avatarSize = avatar.Length();
    auto buffer = std::make_unique<unsigned char[]>( avatarSize );
    avatar.Read( buffer.get(), avatarSize );

    const unsigned char JPEG_MAGIC_NUMBER_ARR[] = { 0xFF, 0xD8, 0xFF, 0xE0 };
    const uint32_t* JPEG_MAGIC_NUMBER = (const uint32_t*)JPEG_MAGIC_NUMBER_ARR;

    const unsigned char JFIF_MAGIC_NUMBER_ARR[] = "JFIF";
    const uint32_t* JFIF_MAGIC_NUMBER = (const uint32_t*)JFIF_MAGIC_NUMBER_ARR;

    bool loResFound = false;

    for ( size_t off = 4; off < avatarSize - sizeof( uint32_t ); off++ )
    {
        // Iterate over file byte-by-byte and try to find JPEG file magic number
        uint32_t* ptr = (uint32_t*)( buffer.get() + off );

        if ( *ptr == *JPEG_MAGIC_NUMBER )
        {
            // JPEG header has probably been found. Verify it by checking 
            // if there is a 'JFIF' string at offset 0x06
            uint32_t* jfifPtr = (uint32_t*)( buffer.get() + off + 0x06 );

            if ( off + 10 < avatarSize && *jfifPtr == *JFIF_MAGIC_NUMBER )
            {
                // Move back four bytes, because there is file size to be read
                uint32_t* sizePtr = (uint32_t*)( buffer.get() + off - 0x04 );

                uint32_t jpegSize = *sizePtr;

                if ( !loResFound )
                {
                    wxMemoryInputStream stream( ptr, jpegSize );
                    this->m_loRes.LoadFile( stream, wxBITMAP_TYPE_JPEG );

                    loResFound = true;
                }
                else 
                {
                    wxMemoryInputStream stream( ptr, jpegSize );
                    this->m_hiRes.LoadFile( stream, wxBITMAP_TYPE_JPEG );

                    // Both JPEGs have been found, so we can safely 
                    // finish our processing.

                    return true;
                }

                off += jpegSize;
            }
        }
    }

    return true;
}

const wxImage& AccountPictureExtractor::getLowResImage() const
{
    return m_loRes;
}

const wxImage& AccountPictureExtractor::getHighResImage() const
{
    return m_hiRes;
}

ExtractorError AccountPictureExtractor::getProcessingError() const
{
    return m_error;
}
