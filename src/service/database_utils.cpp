#include "database_utils.hpp"

#include <algorithm>

#include <memory.h>

namespace srv
{

const std::time_t DatabaseUtils::DAY = 24 * 60 * 60;

std::string DatabaseUtils::getSpecSQLCondition( 
    const std::string fieldName, TimeSpec spec, std::time_t current )
{
    std::tm currTime;

    if ( !current )
    {
        current = time( nullptr );
    }
    
    memcpy( &currTime, localtime( &current ), sizeof( std::tm ) );

    setMidnight( currTime );

    switch ( spec )
    {
    case TimeSpec::TODAY:
    {
        std::time_t greaterThan = std::mktime( &currTime );

        return fieldName + ">=" + std::to_string( greaterThan );
    }
    case TimeSpec::YESTERDAY:
    {
        std::time_t lessThan = std::mktime( &currTime );
        std::time_t greaterThan = lessThan - DAY;

        return (
            fieldName + '<' + std::to_string( lessThan ) + " AND "
            + fieldName + ">=" + std::to_string( greaterThan ) );
    }
    case TimeSpec::THIS_WEEK:
    {
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
        std::time_t today = std::mktime( &currTime );
        std::tm weekStart = getWeekStart( currTime );

        std::time_t yesterdayTime = today - DAY;
        std::time_t weekStartTime = std::mktime( &weekStart );

        // On Monday, yesterdayTime is lower than weekStartTime
        std::time_t lessThan = std::min( yesterdayTime, weekStartTime );
        std::time_t greaterThan = weekStartTime - 7 * DAY;

        return (
            fieldName + '<' + std::to_string( lessThan ) + " AND "
            + fieldName + ">=" + std::to_string( greaterThan ) );
    }
    case TimeSpec::THIS_MONTH:
    {
        std::tm weekStart = getWeekStart( currTime );
        std::time_t weekStartTime = std::mktime( &weekStart );
        std::time_t lastWeekStartTime = weekStartTime - 7 * DAY;

        std::tm monthStart = getMonthStart( currTime );

        std::time_t lessThan = lastWeekStartTime;
        std::time_t greaterThan = std::mktime( &monthStart );

        return (
            fieldName + '<' + std::to_string( lessThan ) + " AND "
            + fieldName + ">=" + std::to_string( greaterThan ) );
    }
    case TimeSpec::LAST_MONTH:
    {
        std::tm weekStart = getWeekStart( currTime );
        std::time_t weekStartTime = std::mktime( &weekStart );
        std::time_t lastWeekStartTime = weekStartTime - 7 * DAY;

        std::tm monthStart = getMonthStart( currTime );
        std::time_t monthStartTime = std::mktime( &monthStart );

        monthStart.tm_mon--;

        // If last week happens to be in the previous month, exclude it
        std::time_t lessThan = std::min( monthStartTime, lastWeekStartTime );
        std::time_t greaterThan = std::mktime( &monthStart );

        return (
            fieldName + '<' + std::to_string( lessThan ) + " AND "
            + fieldName + ">=" + std::to_string( greaterThan ) );
    }
    case TimeSpec::THIS_YEAR:
    {
        std::tm monthStart = getMonthStart( currTime );
        monthStart.tm_mon--;
        std::time_t lastMonthStartTime = std::mktime( &monthStart );
        
        std::tm yearStart = getYearStart( currTime );

        std::time_t lessThan = lastMonthStartTime;
        std::time_t greaterThan = std::mktime( &yearStart );

        return (
            fieldName + '<' + std::to_string( lessThan ) + " AND "
            + fieldName + ">=" + std::to_string( greaterThan ) );
    }
    case TimeSpec::LAST_YEAR:
    {
        std::tm monthStart = getMonthStart( currTime );
        monthStart.tm_mon--;
        std::time_t lastMonthStartTime = std::mktime( &monthStart );

        std::tm yearStart = getYearStart( currTime );
        std::time_t yearStartTime = std::mktime( &yearStart );

        yearStart.tm_year--;

        // In January, the previous months happens to be in the previous year
        std::time_t lessThan = std::min( yearStartTime, lastMonthStartTime );
        std::time_t greaterThan = std::mktime( &yearStart );

        return (
            fieldName + '<' + std::to_string( lessThan ) + " AND "
            + fieldName + ">=" + std::to_string( greaterThan ) );
    }
    case TimeSpec::BEFORE_LAST_YEAR:
    {
        std::tm yearStart = getYearStart( currTime );
        yearStart.tm_year--;
        std::time_t lessThan = std::mktime( &yearStart );

        return fieldName + '<' + std::to_string( lessThan );
    }
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

std::tm DatabaseUtils::getMonthStart( const std::tm& tim )
{
    std::tm out;
    memcpy( &out, &tim, sizeof( std::tm ) );

    out.tm_mday = 1;
    return out;
}

std::tm DatabaseUtils::getYearStart( const std::tm& tim )
{
    std::tm out = getMonthStart( tim );
    out.tm_mon = 0;

    return out;
}

};
