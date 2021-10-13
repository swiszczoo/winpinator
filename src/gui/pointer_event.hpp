#pragma once
#include <wx/wx.h>

#include <memory>

namespace gui
{

class PointerEvent;
wxDECLARE_EVENT( EVT_POINTER, PointerEvent );

typedef void ( wxEvtHandler::*PointerEventFunction )( PointerEvent& );
#define PointerHandler( func ) wxEVENT_HANDLER_CAST( PointerEventFunction, func ) 

class PointerEvent : public wxCommandEvent
{
public:
    explicit PointerEvent( wxEventType commandType = EVT_POINTER, int id = 0 );
    PointerEvent( const PointerEvent& other );

    virtual wxEvent* Clone() const override;

    template<typename T>
    inline void setSharedPointer( std::shared_ptr<T> ptr )
    {
        m_pointer = std::static_pointer_cast<void>( ptr );
    }

    template<typename T>
    inline std::shared_ptr<T> getSharedPointer() const
    {
        return std::static_pointer_cast<T>( m_pointer );
    }

private:
    std::shared_ptr<void> m_pointer;
};

};
