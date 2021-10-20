#pragma once
#include <wx/image.h>


enum class ExtractorError
{
    OK,
    REG_KEY_NOT_FOUND,
    AVATAR_NOT_FOUND,
    AVATAR_FILE_UNREADABLE
};

class AccountPictureExtractor
{
public:
    AccountPictureExtractor();
    bool process();
    const wxImage& getLowResImage() const;
    const wxImage& getHighResImage() const;

    ExtractorError getProcessingError() const;

private:
    static const wxString USER_KEY_NAME;
    static const wxString SYSTEM_KEY_NAME;

    ExtractorError m_error;
    wxImage m_loRes;
    wxImage m_hiRes;

    bool tryExtractingFromUserRegistry();
    bool tryExtractingFromSystemRegistry();

    wxString getCurrentUserSID();
};
