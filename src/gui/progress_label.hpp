#pragma once
#include <wx/wx.h>
#include <wx/animate.h>

namespace gui
{

class ProgressLabel : public wxPanel
{
public:
    ProgressLabel( wxWindow* parent, const wxString& label );

    void setLabel( const wxString& newLabel );
    const wxString& getLabel() const;

private:
    wxStaticText* m_stText;
    wxAnimationCtrl* m_anim;
};

};
