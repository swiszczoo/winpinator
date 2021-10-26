#include "language_adapter.hpp"

#include <wx/filename.h>
#include <wx/stdpaths.h>

namespace gui
{

LanguageAdapter::LanguageAdapter()
    : m_executableFolder( wxEmptyString )
    , m_dataReady( false )
{
    wxString exePath = wxStandardPaths::Get().GetExecutablePath();
    wxFileName exeFname( exePath );
    exeFname.SetFullName( wxEmptyString );

    m_executableFolder = exeFname.GetFullPath();

    openLanguageFile();
}

std::vector<LanguageAdapter::LanguageInfo> LanguageAdapter::getAllLanguages()
{
    if ( !m_dataReady )
    {
        loadData();
    }

    return m_data;
}

const LanguageAdapter::LanguageInfo&
LanguageAdapter::getLanguageInfoByIndex( size_t index )
{
    return m_data.at( index );
}

void LanguageAdapter::openLanguageFile()
{
    wxFileName fname( m_executableFolder, "Languages.xml" );

    bool success = m_langListDoc.Load( fname.GetFullPath() );
}

void LanguageAdapter::loadData()
{
    m_data.clear();

    wxXmlNode* root = m_langListDoc.GetRoot();
    if ( !root )
    {
        return;
    }

    if ( root->GetName() != "Languages" )
    {
        return;
    }

    for ( auto lang = root->GetChildren(); lang; lang = lang->GetNext() )
    {
        if ( lang->GetName() != "Language" )
        {
            return;
        }

        m_data.push_back( parseLanguage( lang ) );
    }

    m_dataReady = true;
}

LanguageAdapter::LanguageInfo LanguageAdapter::parseLanguage( wxXmlNode* node )
{
    LanguageInfo info;
    info.icuCode = node->GetAttribute( "code" );
    info.localName = node->GetAttribute( "name" );

    wxString flagRelativePath = node->GetAttribute( "flag", "zz.png" );
    wxFileName flagFileName( m_executableFolder, flagRelativePath );
    flagFileName.AppendDir( "flags" );

    info.flagPath = flagFileName.GetFullPath();

    return info;
}

};
