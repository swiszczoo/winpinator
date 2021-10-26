#pragma once
#include <wx/file.h>
#include <wx/string.h>
#include <wx/xml/xml.h>

#include <vector>

namespace gui
{

class LanguageAdapter
{
public:
    struct LanguageInfo
    {
        wxString flagPath;
        wxString localName;
        wxString icuCode;
    };

    LanguageAdapter();
    std::vector<LanguageInfo> getAllLanguages();
    const LanguageInfo& getLanguageInfoByIndex( size_t index );

private:
    wxXmlDocument m_langListDoc;
    wxString m_executableFolder;
    bool m_dataReady;
    std::vector<LanguageInfo> m_data;

    void openLanguageFile();
    void loadData();
    LanguageInfo parseLanguage( wxXmlNode* node );
};

};
