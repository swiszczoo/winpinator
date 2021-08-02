#pragma once

#include <wx/wx.h>
#include <wx/dde.h>
#include <wx/event.h>

#include <memory>

wxDECLARE_EVENT( EVT_OPEN_APP_WINDOW, wxCommandEvent );

class WinpinatorDDEServer;

class DDEHandler
{
public:
    virtual void processExecute( const void* data, size_t size ) = 0;
};

class DDEImpl : public wxDDEServer
{
    friend class WinpinatorDDEServer;

public:
    virtual wxConnectionBase* OnAcceptConnection( 
        const wxString& topic ) override;

private:
    DDEHandler* m_handler;

    DDEImpl( DDEHandler* handler );
};

class DDEConnectionImpl : public wxDDEConnection
{
    friend class DDEImpl;

private:
    DDEHandler* m_handler;

    DDEConnectionImpl( DDEHandler* handler );
    virtual bool OnExecute( const wxString& topic,
        const void* data, size_t size, wxIPCFormat format );
};

class WinpinatorDDEServer : DDEHandler
{
public:
    WinpinatorDDEServer( wxEvtHandler* evtHandler );

private:
    wxEvtHandler* m_handler;
    std::shared_ptr<DDEImpl> m_server;

    virtual void processExecute( const void* data, size_t size );
};
