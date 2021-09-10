#pragma once
#include "history_item.hpp"
#include "history_std_bitmaps.hpp"

#include <wx/simplebook.h>
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
    OVERWRITE_NEEDED
};

struct HistoryPendingData
{
    HistoryPendingState opState;
    int64_t opStartTime;

    int numFiles;
    int numFolders;
    wxString singleElementName;

    std::vector<wxString> filePaths;

    long long totalSizeBytes;
    
    bool outcoming;
    float progress;
};

class HistoryPendingElement : public HistoryItem
{
public:
    HistoryPendingElement( wxWindow* parent, HistoryStdBitmaps* bitmaps );

    void setData( const HistoryPendingData& newData );
    const HistoryPendingData& getData() const;

    void setPeerName( const wxString& peerName );
    const wxString& getPeerName() const;

private:
    static const int ICON_SIZE;

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

    wxIcon m_fileIcon;
    wxIconLocation m_fileIconLoc;

    void calculateLayout();
    void setupForState( HistoryPendingState state );

    void onPaint( wxPaintEvent& event );
    void onDpiChanged( wxDPIChangedEvent& event );

    const wxBitmap& determineBitmapToDraw() const;
    wxString determineHeaderString() const;
};

};
