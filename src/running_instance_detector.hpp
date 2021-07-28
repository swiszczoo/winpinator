#pragma once
#include <wx/wx.h>
#include <wx/file.h>

#include <memory>
#include <string>

class RunningInstanceDetector
{
public:
    RunningInstanceDetector( const std::string& lockFileName );
    bool isAnotherInstanceRunning();
    void free();

private:
    typedef uint32_t pid_type;

    std::shared_ptr<wxFile> m_handle;
    bool m_anotherRunning;

    bool checkPidHasOurImageName( pid_type pid );
    bool writeCurrentPidToFile();
};
