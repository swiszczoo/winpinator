#include "pointer_event.hpp"

namespace gui
{

wxDEFINE_EVENT( EVT_POINTER, PointerEvent );

PointerEvent::PointerEvent( 
    wxEventType commandType, int id )
    : wxCommandEvent( commandType, id )
{
}

PointerEvent::PointerEvent( const PointerEvent& other )
    : wxCommandEvent( other )
{
    m_pointer = other.m_pointer;
}

wxEvent* PointerEvent::Clone() const
{
    return new PointerEvent( *this );
}

};
