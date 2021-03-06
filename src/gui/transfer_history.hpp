#pragma once
#include "history_element_finished.hpp"
#include "history_element_pending.hpp"
#include "history_group_header.hpp"
#include "history_item.hpp"
#include "history_std_bitmaps.hpp"
#include "scrollable_restorable.hpp"

#include "../service/service_observer.hpp"

#include <wx/wx.h>

#include <wx/collpane.h>
#include <wx/dnd.h>

#include <vector>

namespace gui
{

wxDECLARE_EVENT( EVT_UPDATE_EMPTY_STATE, wxCommandEvent );

class ScrolledTransferHistory : 
    public wxScrolledWindow, srv::IServiceObserver, public ScrollableRestorable
{
public:
    explicit ScrolledTransferHistory( wxWindow* parent, 
        const wxString& targetId );

    void updateTimeGroups();
    void updateLayout();

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

    template <class T>
    struct TimeGroup
    {
        HistoryGroupHeader* header;
        wxPanel* panel;
        wxBoxSizer* sizer;

        std::vector<int> currentIds;
        std::vector<T*> elements;
    };

    enum class ThreadEventType
    {
        ADD,
        UPDATE,
        REMOVE
    };

    static const std::vector<wxString> TIME_SPECS;

    wxStaticText* m_emptyLabel;
    wxBoxSizer* m_mainSizer;

    TimeGroup<HistoryPendingElement> m_pendingGroup;
    std::vector<TimeGroup<HistoryFinishedElement>> m_timeGroups;

    std::vector<HistoryItem*> m_historyItems;

    HistoryStdBitmaps m_stdBitmaps;

    wxString m_targetId;

    void registerHistoryItem( HistoryItem* item );
    void unregisterHistoryItem( HistoryItem* item );

    void refreshAllHistoryItems( bool insideParent );
    void reloadStdBitmaps();
    void loadSingleBitmap( int resId, wxBitmap* dest, int dip );

    void addPendingTransfer( const srv::TransferOpPub& transfer );
    void updatePendingTransfer( const srv::TransferOpPub& transfer );
    void deletePendingTransfer( int transferId );
    void loadAllTransfers();
    HistoryPendingData convertOpToData( const srv::TransferOpPub& transfer );

    void onScrollWindow( wxScrollWinEvent& event );
    void onMouseEnter( wxMouseEvent& event );
    void onMouseLeave( wxMouseEvent& event );
    void onMouseMotion( wxMouseEvent& event );
    void onDpiChanged( wxDPIChangedEvent& event );
    void onThreadEvent( wxThreadEvent& event );
    void onHistoryItemRemoved( wxCommandEvent& event );

    // Observer methods
    virtual void onStateChanged() override;
    virtual void onAddTransfer( std::string remoteId, 
        srv::TransferOpPub transfer ) override;
    virtual void onUpdateTransfer( std::string remoteId,
        srv::TransferOpPub transfer ) override;
    virtual void onRemoveTransfer( std::string remoteId,
        int transferId ) override;

    virtual void saveScrollPosition() override;
    virtual void restoreScrollPosition() override;

    friend class ScrolledTransferHistory::DropTargetImpl;
};

};
