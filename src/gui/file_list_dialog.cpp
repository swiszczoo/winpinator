#include "file_list_dialog.hpp"

#include "../globals.hpp"

#include <wx/clipbrd.h>
#include <wx/filename.h>
#include <wx/mimetype.h>
#include <wx/msw/uxtheme.h>

#include <stack>

namespace gui
{

const wxString FileListDialog::LABEL_TEXT = wxTRANSLATE(
    "The list below contains all files that were registered as being "
    "sent/received within the transfer you've selected. The list might be "
    "incomplete if the transfer has been interrupted or failed "
    "in a different way." );

FileListDialog::FileListDialog( wxWindow* parent )
    : wxDialog( parent, wxID_ANY, _( "List of transferred files" ),
        wxDefaultPosition, wxDefaultSize,
        wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER )
    , m_transferId( -1 )
    , m_targetId( L"" )
    , m_label( nullptr )
    , m_fileList( nullptr )
    , m_selectedItem( 0 )
{
    wxTRANSLATE( "Close" );

    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
    sizer->AddSpacer( FromDIP( 10 ) );

    m_label = new wxStaticText( this, wxID_ANY, wxGetTranslation( LABEL_TEXT ) );
    sizer->Add( m_label, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    sizer->AddSpacer( FromDIP( 10 ) );

    m_fileList = new wxTreeListCtrl( this, wxID_ANY );
    SetWindowTheme( m_fileList->GetHWND(), L"Explorer", NULL );
    setupColumns();
    sizer->Add( m_fileList, 1, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    sizer->AddSpacer( FromDIP( 10 ) );

    wxSizer* buttonSizer = CreateSeparatedButtonSizer( wxCLOSE );
    sizer->Add( buttonSizer, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 8 ) );

    sizer->AddSpacer( FromDIP( 10 ) );

    SetSizer( sizer );

    sizer->SetSizeHints( this );

    SetMinSize( FromDIP( wxSize( 600, 500 ) ) );
    SetSize( GetMinSize() );
    CenterOnParent();

    // Events
    Bind( wxEVT_INIT_DIALOG, &FileListDialog::onDialogInit, this );
    Bind( wxEVT_SIZE, &FileListDialog::onDialogResized, this );
    m_fileList->Bind( wxEVT_TREELIST_ITEM_CONTEXT_MENU, 
        &FileListDialog::onListRightClicked, this );

    Bind( wxEVT_MENU, &FileListDialog::onCopyNameClicked, 
        this, (int)MenuID::COPY_NAME );
    Bind( wxEVT_MENU, &FileListDialog::onCopyTypeClicked,
        this, (int)MenuID::COPY_TYPE );
    Bind( wxEVT_MENU, &FileListDialog::onCopyRelativeClicked,
        this, (int)MenuID::COPY_RELATIVE );
    Bind( wxEVT_MENU, &FileListDialog::onCopyAbsoluteClicked,
        this, (int)MenuID::COPY_ABSOLUTE );
}

void FileListDialog::setTransferData( int transferId, std::wstring targetId )
{
    m_transferId = transferId;
    m_targetId = targetId;
}

void FileListDialog::updateLabelWrapping()
{
    int size = GetClientSize().x - 2 * FromDIP( 10 );
    m_label->SetLabel( wxGetTranslation( LABEL_TEXT ) );
    m_label->Wrap( size );

    Layout();
}

void FileListDialog::setupColumns()
{
    m_fileList->AppendColumn( _( "File name" ), FromDIP( 150 ), 
        wxALIGN_LEFT, wxCOL_RESIZABLE | wxCOL_SORTABLE );
    m_fileList->AppendColumn( _( "File type" ), FromDIP( 130 ),
        wxALIGN_LEFT, wxCOL_RESIZABLE | wxCOL_SORTABLE );
    m_fileList->AppendColumn( _( "Relative path" ), FromDIP( 225 ),
        wxALIGN_LEFT, wxCOL_RESIZABLE | wxCOL_SORTABLE );
    m_fileList->AppendColumn( _( "Absolute path" ), FromDIP( 400 ),
        wxALIGN_LEFT, wxCOL_RESIZABLE | wxCOL_SORTABLE );
}

void FileListDialog::loadPaths()
{
    auto serv = Globals::get()->getWinpinatorServiceInstance();
    const auto& thisTransfer = serv->getDb()->getTransfer( 
        m_transferId, m_targetId, true );

    struct FolderInfo
    {
        wxTreeListItem item;
        wxString relativePath;
    };

    std::stack<FolderInfo> folderLevels;
    folderLevels.push( { m_fileList->GetRootItem(), wxEmptyString } );

    for ( const auto& path : thisTransfer.elements )
    {
        while ( !folderLevels.empty() )
        {
            const FolderInfo& lastFolder = folderLevels.top();
            if ( wxString( path.relativePath ).Contains( lastFolder.relativePath ) )
            {
                break;
            }
            else
            {
                folderLevels.pop();
            }
        }

        wxTreeListItem currentItem;
        if ( path.elementType == srv::db::TransferElementType::FOLDER )
        {
            const FolderInfo& lastFolder = folderLevels.top();
            currentItem = m_fileList->AppendItem( 
                lastFolder.item, path.elementName );

            FolderInfo newInfo;
            newInfo.item = currentItem;
            newInfo.relativePath = path.relativePath;

            if ( !newInfo.relativePath.EndsWith( "/" ) )
            {
                newInfo.relativePath.Append( "/" );
            }

            folderLevels.push( newInfo );
        }
        else
        {
            const FolderInfo& lastFolder = folderLevels.top();
            currentItem = m_fileList->AppendItem( 
                lastFolder.item, path.elementName );
        }

        m_fileList->SetItemText( currentItem, 1, getElementType( path ) );
        m_fileList->SetItemText( currentItem, 2, path.relativePath );
        m_fileList->SetItemText( currentItem, 3, path.absolutePath );
    }
}

wxString FileListDialog::getElementType( 
    const srv::db::TransferElement& element ) 
{
    if ( element.elementType == srv::db::TransferElementType::FOLDER )
    {
        return _( "Folder" );
    }

    wxFileName fileName( element.elementName );
    wxString extension = fileName.GetExt();

    if ( extension.empty() )
    {
        return _( "File" );
    }
        

    wxFileType* fileType
        = wxTheMimeTypesManager->GetFileTypeFromExtension( extension );

    if ( !fileType )
    {
        return _( "File" );
    }

    wxString result;
    if ( !fileType->GetDescription( &result ) )
    {
        result = _( "File" );
    }
         
    delete fileType;
    return result;
}

void FileListDialog::copyStringToClipboard( const wxString& string )
{
    if ( wxTheClipboard->Open() )
    {
        wxTheClipboard->Clear();
        wxTheClipboard->SetData( new wxTextDataObject( string ) );
        wxTheClipboard->Flush();
        wxTheClipboard->Close();
    }
}

void FileListDialog::onDialogInit( wxInitDialogEvent& event )
{
    updateLabelWrapping();
    loadPaths();
}

void FileListDialog::onDialogResized( wxSizeEvent& event )
{
    updateLabelWrapping();
}

void FileListDialog::onListRightClicked( wxTreeListEvent& event ) 
{
    wxMenu menu;
    m_selectedItem = event.GetItem();

    menu.Append( (int)MenuID::COPY_NAME, _( "Copy file name..." ) );
    menu.Append( (int)MenuID::COPY_TYPE, _( "Copy file type..." ) );
    menu.Append( (int)MenuID::COPY_RELATIVE, _( "Copy relative path..." ) );
    menu.Append( (int)MenuID::COPY_ABSOLUTE, _( "Copy absolute path..." ) );

    PopupMenu( &menu );
}

void FileListDialog::onCopyNameClicked( wxCommandEvent& event )
{
    copyStringToClipboard( m_fileList->GetItemText( m_selectedItem, 0 ) );
}

void FileListDialog::onCopyTypeClicked( wxCommandEvent& event )
{
    copyStringToClipboard( m_fileList->GetItemText( m_selectedItem, 1 ) );
}

void FileListDialog::onCopyRelativeClicked( wxCommandEvent& event )
{
    copyStringToClipboard( m_fileList->GetItemText( m_selectedItem, 2 ) );
}

void FileListDialog::onCopyAbsoluteClicked( wxCommandEvent& event )
{
    copyStringToClipboard( m_fileList->GetItemText( m_selectedItem, 3 ) );
}

};
