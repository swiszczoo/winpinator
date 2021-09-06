#pragma once
#include "history_item.hpp"

#include <wx/wx.h>

#include <stdint.h>

namespace gui
{

enum class HistoryPendingState
{
    AWAIT_PEER_APPROVAL,
    AWAIT_MY_APPROVAL,
    TRANSFER_RUNNING,
    TRANSFER_PAUSED
};

struct HistoryPendingData
{
    HistoryPendingState opState;
    int64_t opStartTime;

    int numFiles;
    int numFolders;
    wxString singleElementName;
};

class HistoryPendingElement : public HistoryItem
{
public:
    HistoryPendingElement( wxWindow* parent );

private:

};

};
