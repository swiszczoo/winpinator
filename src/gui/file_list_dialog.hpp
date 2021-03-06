#pragma once
#include "../service/database_types.hpp"

#include <wx/treelist.h>
#include <wx/wx.h>

namespace gui
{

class FileListDialog : public wxDialog
{
public:
    explicit FileListDialog( wxWindow* parent );

    void setTransferData( int transferId, std::wstring targetId );

private:
    enum class MenuID
    {
        COPY_NAME = 1,
        COPY_TYPE,
        COPY_RELATIVE,
        COPY_ABSOLUTE
    };

    static const wxString LABEL_TEXT;

    int m_transferId;
    std::wstring m_targetId;

    wxStaticText* m_label;
    wxTreeListCtrl* m_fileList;

    wxTreeListItem m_selectedItem;

    void updateLabelWrapping();
    void setupColumns();
    void loadPaths();
    static wxString getElementType( const srv::db::TransferElement& element );
    static void copyStringToClipboard( const wxString& string );

    void onDialogInit( wxInitDialogEvent& event );
    void onDialogResized( wxSizeEvent& event );
    void onListRightClicked( wxTreeListEvent& event );

    void onCopyNameClicked( wxCommandEvent& event );
    void onCopyTypeClicked( wxCommandEvent& event );
    void onCopyRelativeClicked( wxCommandEvent& event );
    void onCopyAbsoluteClicked( wxCommandEvent& event );
};

};
