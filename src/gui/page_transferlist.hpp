#pragma once
#include <wx/wx.h>

#include "tool_button.hpp"

namespace gui
{

class TransferListPage : public wxPanel
{
public:
    explicit TransferListPage( wxWindow* parent );

private:
    wxStaticText* m_header;
    wxStaticText* m_details;
    ToolButton* m_backBtn;
    ToolButton* m_fileBtn;
    ToolButton* m_directoryBtn;
    wxScrolledWindow* m_opList;

    wxBitmap m_backBmp;
    wxBitmap m_fileBmp;
    wxBitmap m_dirBmp;

    void loadIcons();
    void loadSingleIcon( const wxString& res, wxBitmap* loc, ToolButton* btn );

    void onDpiChanged( wxDPIChangedEvent& event );
};

};
