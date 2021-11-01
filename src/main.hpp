#pragma once
#include "gui/winpinator_frame.hpp"
#include "running_instance_detector.hpp"
#include "tray_icon.hpp"
#include "winpinator_dde_server.hpp"

#include <wx/translation.h>
#include <wx/wx.h>

#include <memory>
#include <thread>

#define NO_DECLARE_APP
#include "main_base.hpp"

class WinpinatorApp : public WinpinatorAppBase, srv::IServiceObserver
{
public:
    WinpinatorApp();
    virtual bool OnInit() override;
    virtual int OnExit() override;

private:
    enum class EventType
    {
        STATE_CHANGED,
        OPEN_TRANSFER_UI
    };

    gui::WinpinatorFrame* m_topLvl;
    TrayIcon* m_trayIcon;
    std::thread m_srvThread;

    std::shared_ptr<RunningInstanceDetector> m_detector;
    std::unique_ptr<WinpinatorDDEServer> m_ddeServer;

    void showMainFrame();
    void onMainFrameDestroyed( wxWindowDestroyEvent& event );
    void onDDEOpenCalled( wxCommandEvent& event );
    void onRestore( wxCommandEvent& event );
    void onServiceEvent( wxThreadEvent& event );
    void onExitApp( wxCommandEvent& event );
    void onOpenSaveFolder( wxCommandEvent& event );

    // Service observer methods:
    virtual void onStateChanged();
    virtual void onOpenTransferUI( wxString remoteId );
};

wxDECLARE_APP( WinpinatorApp );
