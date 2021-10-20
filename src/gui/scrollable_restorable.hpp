#pragma once
#include <wx/wx.h>

namespace gui
{

class ScrollableRestorable
{
public:
    virtual void saveScrollPosition() = 0;
    virtual void restoreScrollPosition() = 0;

protected:
    wxPoint m_scrollPos;
};

};
