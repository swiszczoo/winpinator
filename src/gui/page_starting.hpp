#pragma once
#include <wx/wx.h>
#include <wx/activityindicator.h>

namespace gui
{

class StartingPage : public wxPanel
{
public:
    explicit StartingPage( wxWindow* parent );

private:
    wxActivityIndicator* m_indicator;
    wxStaticText* m_label;
};

};
