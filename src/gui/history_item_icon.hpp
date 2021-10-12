#pragma once
#include "history_item.hpp"
#include "history_std_bitmaps.hpp"

namespace gui
{

class HistoryIconItem : public HistoryItem
{
public:
    explicit HistoryIconItem( wxWindow* parent, HistoryStdBitmaps* bmps );

    void setIsLast( bool isLast );
    bool isLast() const;

protected:
    void setIcons( int folderCount, int fileCount, const wxString& fileExt );
    void setOutcoming( bool outcoming );
    void setFinished( bool finished );

    const wxBitmap& determineBitmapToDraw() const;
    wxString determineHeaderString() const;

    wxString getElementType() const;

    wxCoord drawIcon( wxPaintDC& dc ); // Returns content offset in pixels

private:
    static const int ICON_SIZE;

    HistoryStdBitmaps* m_bitmaps;
    wxString m_singleElementName;
    int m_folderCount;
    int m_fileCount;
    bool m_outcoming;
    bool m_last;
    bool m_finished;

    wxIcon m_fileIcon;
    wxIconLocation m_fileIconLoc;

    wxString m_elementType;

    void onDpiChanged( wxDPIChangedEvent& event );
    void setupElementType();
};

};
