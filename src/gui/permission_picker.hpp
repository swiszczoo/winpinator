#pragma once
#include <wx/wx.h>

namespace gui
{

class PermissionPicker : public wxPanel
{
public:
    explicit PermissionPicker( wxWindow* parent, bool directory );

    void setLabel( const wxString& label );
    wxString getLabel() const;

    void setLabelWidth( int width );
    int getLabelWidth() const;

    void setPermissionMask( int mask );
    int getPermissionMask() const;

private:
    wxCheckBox* m_checkboxes[9];
    wxSizerItem* m_offset;
    wxString m_label;
    wxStaticText* m_labelCtrl;
    int m_labelWidth;
};

};
