#pragma once
#include <wx/wx.h>

namespace gui
{

class SpritesheetAnimator : public wxWindow
{
public:
    explicit SpritesheetAnimator( wxWindow* parent );
    ~SpritesheetAnimator();

    void setBitmap( const wxBitmap& bmp );
    const wxBitmap& getBitmap() const;

    void setIntervalMs( int millis );
    int getIntervalMs() const;

    void startAnimation();
    void stopAnimation();
    bool isPlaying() const;

    void DoEnable( bool enable ) override;

private:
    wxBitmap m_bitmap;
    wxMemoryDC m_memDc;
    wxTimer* m_timer;
    int m_interval;
    int m_currentFrame;
    bool m_playing;

    void onPaint( wxPaintEvent& event );
    void onSize( wxSizeEvent& event );
    void onTimerTick( wxTimerEvent& event );
};

};
