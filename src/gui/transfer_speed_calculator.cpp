#include "transfer_speed_calculator.hpp"

#include <algorithm>
#include <chrono>

namespace gui
{

const int TransferSpeedCalculator::WEIGHT_RAMP_MILLIS = 2000;
const int TransferSpeedCalculator::MIN_DATA_POINTS = 4;

TransferSpeedCalculator::TransferSpeedCalculator()
    : m_totalBytes( 1000000 )
    , m_bytesPerSec( 0 )
    , m_remainingSeconds( -1 )
{
}

void TransferSpeedCalculator::reset(
    long long currentBytes, long long totalBytes )
{
    m_points.clear();
    m_timePoints.clear();
    m_totalBytes = totalBytes;

    update( currentBytes );
}

void TransferSpeedCalculator::update( long long currentBytes )
{
    using namespace std::chrono;
    auto time = steady_clock::now();

    removeOldestPoints( time );

    DataPoint point;
    point.bytes = currentBytes;
    point.time = time;
    m_points.push_back( point );

    recalculateSpeedAndTime();
}

int TransferSpeedCalculator::getRemainingTimeInSeconds() const
{
    return m_remainingSeconds;
}

long long TransferSpeedCalculator::getTransferSpeedInBps() const
{
    return m_bytesPerSec;
}

void TransferSpeedCalculator::removeOldestPoints(
    std::chrono::steady_clock::time_point time )
{
    using namespace std::chrono;

    auto notAfter = time - milliseconds( WEIGHT_RAMP_MILLIS );
    auto it = m_points.begin();

    while ( m_points.size() > MIN_DATA_POINTS && it != m_points.end() )
    {
        if ( it->time >= notAfter )
        {
            break;
        }
        else
        {
            it = m_points.erase( it );
        }
    }
}

void TransferSpeedCalculator::recalculateSpeedAndTime()
{
    using namespace std::chrono;

    m_bytesPerSec = 0;
    m_remainingSeconds = -1;

    auto currentTime = m_points.back().time;
    auto rampStartTime = m_points.front().time;

    int rampMillis = duration_cast<milliseconds>(
        currentTime - rampStartTime )
                         .count();

    if ( rampMillis < WEIGHT_RAMP_MILLIS )
    {
        rampMillis = WEIGHT_RAMP_MILLIS;
    }

    auto current = m_points.begin();
    auto next = ++m_points.begin();

    double totalWeight = 0.0;
    double totalWeightedBytes = 0.0;

    while ( next != m_points.end() )
    {
        long long fragEndTime = duration_cast<milliseconds>(
            currentTime - next->time )
                                    .count();
        long long fragStartTime = duration_cast<milliseconds>(
            currentTime - current->time )
                                      .count();

        int fragMillis = abs( fragStartTime - fragEndTime );

        if ( fragMillis == 0 )
        {
            current++;
            next++;
            continue;
        }

        long long fragBytes = abs( next->bytes - current->bytes );
        double fragCenterMillis = ( fragStartTime + fragEndTime ) / 2.0;

        double weightTime = 1 - ( (double)fragCenterMillis / rampMillis );
        double weightLength = fragMillis / 100.0;

        double weight = weightTime * weightLength;
        double bytesPerSec = fragBytes / (double)fragMillis * 1000.0;

        totalWeight += weight;
        totalWeightedBytes += bytesPerSec * weight;

        current++;
        next++;
    }

    if ( totalWeight == 0.0 )
    {
        return;
    }

    m_bytesPerSec = (long long)round( totalWeightedBytes / totalWeight );
    calculateTime( m_points.back().bytes );
}

void TransferSpeedCalculator::calculateTime( long long current )
{
    using namespace std::chrono;

    if ( m_points.size() < MIN_DATA_POINTS )
    {
        m_timePoints.clear();
        return;
    }

    long long remaining = m_totalBytes - current;
    long long targetSeconds
        = (long long)round( remaining / (double)m_bytesPerSec );

    DataPoint currPoint;
    currPoint.bytes = targetSeconds;
    currPoint.time = steady_clock::now();

    m_timePoints.push_back( currPoint );

    double totalWeight = 0.0;
    double totalSeconds = 0.0;

    while ( m_timePoints.size() > 5 )
    {
        m_timePoints.pop_front();
    }

    for ( DataPoint& pnt : m_timePoints )
    {
        int secondsSincePoint = duration_cast<seconds>(
            currPoint.time - pnt.time )
                                    .count();

        totalWeight += 1.0;
        totalSeconds += std::max( 0LL, pnt.bytes - secondsSincePoint );
        
        totalWeight *= 0.5;
        totalSeconds *= 0.5;
    }

    if ( totalWeight > 0 )
    {
        m_remainingSeconds = (int)round( totalSeconds / totalWeight );
    }
    else
    {
        m_remainingSeconds = -1;
    }
}

};
