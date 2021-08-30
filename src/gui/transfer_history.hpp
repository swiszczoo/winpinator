#pragma once
#include "history_group_header.hpp"

#include <wx/wx.h>

#include <wx/collpane.h>
#include <wx/dnd.h>

#include <vector>

namespace gui
{

class ScrolledTransferHistory : public wxScrolledWindow
{
public:
    ScrolledTransferHistory( wxWindow* parent );

private:
    class DropTargetImpl : public wxFileDropTarget
    {
    public:
        DropTargetImpl( ScrolledTransferHistory* obj );
        virtual bool OnDropFiles( wxCoord x, wxCoord y,
            const wxArrayString& filenames );

    private:
        ScrolledTransferHistory* m_instance;
    };

    static const std::vector<wxString> TIME_SPECS;

    wxStaticText* m_emptyLabel;
    std::vector<HistoryGroupHeader*> m_timeHeaders;
    std::vector<wxPanel*> m_timeGroups;
    std::vector<wxBoxSizer*> m_timeSizers;

    friend class ScrolledTransferHistory::DropTargetImpl;
};

};
