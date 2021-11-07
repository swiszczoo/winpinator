#include "file_list_dialog.hpp"

#include "../globals.hpp"

#include <wx/clipbrd.h>
#include <wx/filename.h>
#include <wx/mimetype.h>
#include <wx/msw/uxtheme.h>

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
    , m_selectedId( 0 )
{
    wxTRANSLATE( "Close" );

    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
    sizer->AddSpacer( FromDIP( 10 ) );

    m_label = new wxStaticText( this, wxID_ANY, wxGetTranslation( LABEL_TEXT ) );
    sizer->Add( m_label, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    sizer->AddSpacer( FromDIP( 10 ) );

    m_fileList = new wxListCtrl( this, wxID_ANY, wxDefaultPosition, 
        wxDefaultSize, 
        wxLC_REPORT | wxLC_HRULES | wxLC_VRULES | wxLC_SINGLE_SEL );
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
    m_fileList->Bind( wxEVT_LIST_ITEM_RIGHT_CLICK, 
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
    wxListItem col;

    col.SetColumn( 0 );
    col.SetText( _( "File name" ) );
    col.SetWidth( FromDIP( 100 ) );
    m_fileList->InsertColumn( 0, col );

    col.SetColumn( 1 );
    col.SetText( _( "File type" ) );
    col.SetWidth( FromDIP( 150 ) );
    m_fileList->InsertColumn( 1, col );

    col.SetColumn( 2 );
    col.SetText( _( "Relative path" ) );
    col.SetWidth( FromDIP( 225 ) );
    m_fileList->InsertColumn( 2, col );

    col.SetColumn( 3 );
    col.SetText( _( "Absolute path" ) );
    col.SetWidth( FromDIP( 400 ) );
    m_fileList->InsertColumn( 3, col );
}

void FileListDialog::loadPaths()
{
    auto serv = Globals::get()->getWinpinatorServiceInstance();
    const auto& thisTransfer = serv->getDb()->getTransfer( 
        m_transferId, m_targetId, true );

    int i = 0;
    for ( const auto& path : thisTransfer.elements )
    {
        wxListItem fileName;
        fileName.SetId( i );
        fileName.SetColumn( 0 );
        fileName.SetText( path.elementName );
        m_fileList->InsertItem( fileName );

        wxString elementType = getElementType( path );
        m_fileList->SetItem( i, 1, elementType );

        m_fileList->SetItem( i, 2, path.relativePath );
        m_fileList->SetItem( i, 3, path.absolutePath );

        i++;
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

void FileListDialog::onListRightClicked( wxListEvent& event ) 
{
    wxMenu menu;
    m_selectedId = event.GetIndex();

    menu.Append( (int)MenuID::COPY_NAME, _( "Copy file name..." ) );
    menu.Append( (int)MenuID::COPY_TYPE, _( "Copy file type..." ) );
    menu.Append( (int)MenuID::COPY_RELATIVE, _( "Copy relative path..." ) );
    menu.Append( (int)MenuID::COPY_ABSOLUTE, _( "Copy absolute path..." ) );

    PopupMenu( &menu );
}

void FileListDialog::onCopyNameClicked( wxCommandEvent& event )
{
    copyStringToClipboard( m_fileList->GetItemText( m_selectedId, 0 ) );
}

void FileListDialog::onCopyTypeClicked( wxCommandEvent& event )
{
    copyStringToClipboard( m_fileList->GetItemText( m_selectedId, 1 ) );
}

void FileListDialog::onCopyRelativeClicked( wxCommandEvent& event )
{
    copyStringToClipboard( m_fileList->GetItemText( m_selectedId, 2 ) );
}

void FileListDialog::onCopyAbsoluteClicked( wxCommandEvent& event )
{
    copyStringToClipboard( m_fileList->GetItemText( m_selectedId, 3 ) );
}

};
