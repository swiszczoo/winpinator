#pragma once
#include "history_item_icon.hpp"
#include "history_std_bitmaps.hpp"
#include "pointer_event.hpp"

#include "../service/database_types.hpp"

#include <wx/wx.h>

namespace gui
{

typedef srv::db::Transfer TransferData;

wxDECLARE_EVENT( EVT_REMOVE, wxCommandEvent );
wxDECLARE_EVENT( EVT_OPEN_DIALOG, PointerEvent );
wxDECLARE_EVENT( EVT_CLOSE_DIALOG, PointerEvent );

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
        ID_OPEN = 1,
        ID_EXPLORER,
        ID_PATHS,
        ID_REMOVE
    };

    HistoryStdBitmaps* m_bitmaps;
    TransferData m_data;
    wxFont m_bold;
    wxBrush m_brush;

    void onPaint( wxPaintEvent& event );
    void onDoubleClicked( wxMouseEvent& event );
    void onContextMenu( wxContextMenuEvent& event );

    void onOpenClicked( wxCommandEvent& event );
    void onShowInExplorerClicked( wxCommandEvent& event );
    void onShowListClicked( wxCommandEvent& event );
    void onRemoveClicked( wxCommandEvent& event );

    wxString determineStatusString();
    const wxBitmap& setupStatusDrawing( wxDC* dc );

    void openElement();
};

};
