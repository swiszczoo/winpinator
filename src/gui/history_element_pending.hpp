#pragma once
#include "history_item_icon.hpp"
#include "history_std_bitmaps.hpp"
#include "scrollable_restorable.hpp"
#include "transfer_speed_calculator.hpp"

#include <wx/wx.h>

#include <vector>

#include <stdint.h>

namespace gui
{

enum class HistoryPendingState
{
    AWAIT_PEER_APPROVAL,
    AWAIT_MY_APPROVAL,
    TRANSFER_RUNNING,
    TRANSFER_PAUSED,
    OVERWRITE_NEEDED,
    DISK_FULL
};

struct HistoryPendingData
{
    int transferId;

    HistoryPendingState opState;
    int64_t opStartTime;

    int numFiles;
    int numFolders;
    wxString singleElementName;

    std::vector<wxString> filePaths;

    long long totalSizeBytes;
    
    bool outcoming;
    long long sentBytes;
};

class HistoryPendingElement : public HistoryIconItem
{
public:
    explicit HistoryPendingElement( wxWindow* parent, HistoryStdBitmaps* bitmaps );

    void setData( const HistoryPendingData& newData );
    const HistoryPendingData& getData() const;

    void setPeerName( const wxString& peerName );
    const wxString& getPeerName() const;

    void setRemoteId( const std::string& remoteId );
    const std::string& getRemoteId() const;

    void setScrollableRestorable( ScrollableRestorable* restorable );

    void updateProgress( long long sentBytes );

private:
    static const int ICON_SIZE;
    static const int PROGRESS_RANGE;

    // Await peer approval
    wxBoxSizer* m_info;
    wxString m_infoLabel;
    wxGauge* m_infoProgress;
    wxBoxSizer* m_buttonSizer;
    wxButton* m_infoCancel;
    wxButton* m_infoAllow;
    wxButton* m_infoReject;
    wxButton* m_infoPause;
    wxButton* m_infoStop;
    wxButton* m_infoOverwrite;
    int m_infoSpacing;

    // Other stuff

    HistoryStdBitmaps* m_bitmaps;
    HistoryPendingData m_data;
    wxString m_peerName;
    std::string m_remoteId;
    ScrollableRestorable* m_scrollable;

    TransferSpeedCalculator m_calculator;

    void calculateLayout();
    void setupForState( HistoryPendingState state );

    void onPaint( wxPaintEvent& event );
    void onAllowClicked( wxCommandEvent& event );
    void onDeclineClicked( wxCommandEvent& event );
    void onPauseClicked( wxCommandEvent& event );
    void onCancelClicked( wxCommandEvent& event );

    int calculateRemainingSeconds() const;
    int calculateTransferSpeed() const;
    void disableAllButtons();
};

};
