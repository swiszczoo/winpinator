#pragma once
#include "history_element_finished.hpp"
#include "history_group_header.hpp"
#include "history_item.hpp"
#include "history_std_bitmaps.hpp"

#include <wx/wx.h>

#include <wx/collpane.h>
#include <wx/dnd.h>

#include <vector>

namespace gui
{

class ScrolledTransferHistory : public wxScrolledWindow
{
public:
    explicit ScrolledTransferHistory( wxWindow* parent );

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

    struct TimeGroup
    {
        HistoryGroupHeader* header;
        wxPanel* panel;
        wxBoxSizer* sizer;

        std::vector<int> currentIds;
        std::vector<HistoryFinishedElement*> elements;
    };

    static const std::vector<wxString> TIME_SPECS;

    wxStaticText* m_emptyLabel;

    TimeGroup m_pendingGroup;
    std::vector<TimeGroup> m_timeGroups;

    std::vector<HistoryItem*> m_historyItems;

    HistoryStdBitmaps m_stdBitmaps;

    void registerHistoryItem( HistoryItem* item );
    void unregisterHistoryItem( HistoryItem* item );

    void refreshAllHistoryItems( bool insideParent );
    void reloadStdBitmaps();
    void loadSingleBitmap( int resId, wxBitmap* dest, int dip );

    void updateTimeGroups();

    void onScrollWindow( wxScrollWinEvent& event );
    void onMouseEnter( wxMouseEvent& event );
    void onMouseLeave( wxMouseEvent& event );
    void onMouseMotion( wxMouseEvent& event );
    void onDpiChanged( wxDPIChangedEvent& event );

    friend class ScrolledTransferHistory::DropTargetImpl;
};

};
