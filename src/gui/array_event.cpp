#include "array_event.hpp"

namespace gui
{

wxDEFINE_EVENT( EVT_ARRAY, ArrayEvent );

ArrayEvent::ArrayEvent(
    wxEventType commandType, int id )
    : wxCommandEvent( commandType, id )
{
}

ArrayEvent::ArrayEvent( const ArrayEvent& other )
    : wxCommandEvent( other )
{
    m_vector = other.m_vector;
}

wxEvent* ArrayEvent::Clone() const
{
    return new ArrayEvent( *this );
}

};
