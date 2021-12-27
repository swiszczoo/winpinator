#pragma once
#include <wx/event.h>

#include <memory>
#include <vector>

namespace gui
{

class ArrayEvent;
wxDECLARE_EVENT( EVT_ARRAY, ArrayEvent );

typedef void ( wxEvtHandler::*ArrayEventFunction )( ArrayEvent& );
#define ArrayHandler( func ) wxEVENT_HANDLER_CAST( ArrayEventFunction, func ) 

class ArrayEvent : public wxCommandEvent
{
public:
    explicit ArrayEvent( wxEventType commandType, int id = 0 );
    ArrayEvent( const ArrayEvent& other );

    virtual wxEvent* Clone() const override;

    template <typename T>
    inline void setArray( const std::vector<T>& vector )
    {
        std::shared_ptr<std::vector<T>> clone 
            = std::make_shared<std::vector<T>>( vector );

        m_vector = std::static_pointer_cast<void>( clone );
    }

    template <typename T>
    inline const std::vector<T>& getArray()
    {
        const std::shared_ptr<std::vector<T>>& casted 
            = std::static_pointer_cast<std::vector<T>>( m_vector );

        return *casted;
    }

private:
    std::shared_ptr<void> m_vector;
};

};
