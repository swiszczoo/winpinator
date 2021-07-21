#include "spritesheet_animator.hpp"

namespace gui
{

SpritesheetAnimator::SpritesheetAnimator( wxWindow* parent )
    : wxWindow( parent, wxID_ANY )
    , m_bitmap( wxNullBitmap )
    , m_timer( nullptr )
    , m_interval( 100 )
    , m_currentFrame( 0 )
    , m_playing( true )
{
    m_memDc.SelectObjectAsSource( m_bitmap );
    m_timer = new wxTimer( this );
    m_timer->Start( m_interval );

    Bind( wxEVT_PAINT, &SpritesheetAnimator::onPaint, this );
    Bind( wxEVT_SIZE, &SpritesheetAnimator::onSize, this );
    Bind( wxEVT_TIMER, &SpritesheetAnimator::onTimerTick, this );
}

SpritesheetAnimator::~SpritesheetAnimator()
{
    m_timer->Stop();
}

void SpritesheetAnimator::setBitmap( const wxBitmap& bmp )
{
    m_bitmap = bmp;
    m_memDc.SelectObjectAsSource( m_bitmap );

    Refresh();
}

const wxBitmap& SpritesheetAnimator::getBitmap() const
{
    return m_bitmap;
}

void SpritesheetAnimator::setIntervalMs( int millis )
{
    m_interval = millis;

    if ( m_playing )
    {
        m_timer->Start( millis );
    }
}

int SpritesheetAnimator::getIntervalMs() const
{
    return m_interval;
}

void SpritesheetAnimator::startAnimation()
{
    m_playing = true;
    m_timer->Start( m_interval );
}

void SpritesheetAnimator::stopAnimation()
{
    m_playing = false;
    m_timer->Stop();
}

bool SpritesheetAnimator::isPlaying() const
{
    return m_playing;
}

void SpritesheetAnimator::DoEnable( bool enable )
{
    wxWindow::DoEnable( enable );

    if ( enable )
    {
        startAnimation();
    }
    else
    {
        stopAnimation();
    }
}

void SpritesheetAnimator::onPaint( wxPaintEvent& event )
{
    wxPaintDC dc( this );
    wxSize size = dc.GetSize();
    wxSize bmpSize = m_bitmap.GetSize();

    if ( m_currentFrame * bmpSize.GetHeight() >= bmpSize.GetWidth() )
    {
        m_currentFrame = 0;
    }

    wxPoint srcPnt( m_currentFrame * bmpSize.GetHeight(), 0 );
    wxSize srcSize( bmpSize.GetHeight(), bmpSize.GetHeight() );
    
    dc.Blit( wxPoint(), srcSize, &m_memDc, srcPnt, wxCOPY );
}

void SpritesheetAnimator::onSize( wxSizeEvent& event )
{
    Refresh();
}

void SpritesheetAnimator::onTimerTick( wxTimerEvent& event )
{
    m_currentFrame++;
    Refresh();
}

};
