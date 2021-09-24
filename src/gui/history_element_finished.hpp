#pragma once
#include "history_item.hpp"
#include "history_std_bitmaps.hpp"

#include "../service/database_types.hpp"

#include <wx/wx.h>

namespace gui
{

typedef srv::db::Transfer TransferData;

class HistoryFinishedElement : public HistoryItem
{
public:
    explicit HistoryFinishedElement( wxWindow* parent, HistoryStdBitmaps* bitmaps );

    void setData( const TransferData& newData );
    const TransferData& getData() const;

private:
    static const int ICON_SIZE;

    HistoryStdBitmaps* m_bitmaps;
    TransferData m_data;

    wxIcon m_fileIcon;
    wxIconLocation m_fileIconLoc;
};

};
