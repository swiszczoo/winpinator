#pragma once
#include "history_item_icon.hpp"
#include "history_std_bitmaps.hpp"

#include "../service/database_types.hpp"

#include <wx/wx.h>

namespace gui
{

typedef srv::db::Transfer TransferData;

class HistoryFinishedElement : public HistoryIconItem
{
public:
    explicit HistoryFinishedElement( wxWindow* parent, HistoryStdBitmaps* bitmaps );

    void setData( const TransferData& newData );
    const TransferData& getData() const;

    

private:
    static const int ICON_SIZE;

    enum class MenuID
    {
        ID_EXPLORER,
        ID_PATHS,
        ID_REMOVE
    };

    HistoryStdBitmaps* m_bitmaps;
    TransferData m_data;

    void onPaint( wxPaintEvent& event );
    void onContextMenu( wxContextMenuEvent& event );
};

};
