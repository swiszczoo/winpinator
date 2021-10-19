#pragma once
#include <chrono>
#include <list>

namespace gui
{

class TransferSpeedCalculator
{
public:
    explicit TransferSpeedCalculator();

    void reset( long long currentBytes, long long totalBytes );
    void update( long long currentBytes );

    long long getTransferSpeedInBps() const;
    int getRemainingTimeInSeconds() const;

private:
    struct DataPoint
    {
        long long bytes;
        std::chrono::steady_clock::time_point time;
    };

    static const int WEIGHT_RAMP_MILLIS;
    static const int MIN_DATA_POINTS;

    std::list<DataPoint> m_points;
    std::list<DataPoint> m_timePoints;

    long long m_totalBytes;
    long long m_bytesPerSec;
    int m_remainingSeconds;

    void removeOldestPoints( std::chrono::steady_clock::time_point time );
    void recalculateSpeedAndTime();
    void calculateTime( long long current );
};

};
