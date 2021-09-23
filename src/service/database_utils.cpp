#include "database_utils.hpp"

#include <algorithm>

#include <memory.h>

namespace srv
{

const std::time_t DatabaseUtils::DAY = 24 * 60 * 60;

std::string DatabaseUtils::getSpecSQLCondition( 
    const std::string fieldName, TimeSpec spec )
{
    std::tm currTime;
    std::time_t currSeconds = time( nullptr );
    memcpy( &currTime, localtime( &currSeconds ), sizeof( std::tm ) );

    switch ( spec )
    {
    case TimeSpec::TODAY:
    {
        setMidnight( currTime );
        std::time_t greaterThan = std::mktime( &currTime );

        return fieldName + ">=" + std::to_string( greaterThan );
    }
    case TimeSpec::YESTERDAY:
    {
        setMidnight( currTime );
        std::time_t lessThan = std::mktime( &currTime );
        std::time_t greaterThan = lessThan - DAY;

        return (
            fieldName + '<' + std::to_string( lessThan ) + " AND "
            + fieldName + ">=" + std::to_string( greaterThan ) );
    }
    case TimeSpec::THIS_WEEK:
    {
        setMidnight( currTime );
        std::time_t today = std::mktime( &currTime );
        std::tm weekStart = getWeekStart( currTime );

        std::time_t lessThan = today - DAY;
        std::time_t greaterThan = std::mktime( &weekStart );

        return (
            fieldName + '<' + std::to_string( lessThan ) + " AND "
            + fieldName + ">=" + std::to_string( greaterThan ) );
    }
    case TimeSpec::LAST_WEEK:
    {
        setMidnight( currTime );
        std::tm weekStart = getWeekStart( currTime );

        std::time_t lessThan = std::mktime( &weekStart );
        std::time_t greaterThan = lessThan - 7 * DAY;

        return (
            fieldName + '<' + std::to_string( lessThan ) + " AND "
            + fieldName + ">=" + std::to_string( greaterThan ) );
    }
    // TODO: implement the rest of time specs
    }

    return "FALSE"; // Return false so that no record fulfills this condition
}

void DatabaseUtils::setMidnight( std::tm& tim )
{
    tim.tm_hour = 0;
    tim.tm_min = 0;
    tim.tm_sec = 0;
}

std::tm DatabaseUtils::getWeekStart( const std::tm& tim )
{
    std::tm out;
    memcpy( &out, &tim, sizeof( std::tm ) );

    if ( out.tm_wday == 0 )
    {
        // Sunday is the last day of week
        out.tm_wday = 7;
    }

    out.tm_mday -= out.tm_wday - 1;
    return out;
}

};
