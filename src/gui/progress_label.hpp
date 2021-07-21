#pragma once
#include <wx/wx.h>

#include "spritesheet_animator.hpp"


namespace gui
{

class ProgressLabel : public wxPanel
{
public:
    ProgressLabel( wxWindow* parent, const wxString& label );

    void setLabel( const wxString& newLabel );
    const wxString& getLabel() const;

private:
    wxBitmap m_bitmap;
    wxStaticText* m_stText;
    SpritesheetAnimator* m_anim;

    void loadImages();
    void onDpiChanged( wxDPIChangedEvent& event );
};

};
