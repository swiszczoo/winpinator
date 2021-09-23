#pragma once
#include <chrono>
#include <string>

namespace srv
{

enum class TimeSpec
{
    TODAY,
    YESTERDAY,
    THIS_WEEK,
    LAST_WEEK,
    THIS_MONTH,
    LAST_MONTH,
    THIS_YEAR,
    LAST_YEAR,
    BEFORE_LAST_YEAR
};

class DatabaseUtils
{
public:
    static std::string getSpecSQLCondition( 
        const std::string fieldName, TimeSpec spec );

private:
    static const std::time_t DAY;
    static void setMidnight( std::tm& tim );
    static std::tm getWeekStart( const std::tm& tim );
};

};
