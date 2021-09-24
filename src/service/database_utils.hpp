#pragma once
#include <chrono>
#include <string>

namespace srv
{

enum class TimeSpec
{
    IN_THE_FUTURE,
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
    DatabaseUtils() = delete; // Remove default constructor
    static std::string getSpecSQLCondition(
        const std::string fieldName, TimeSpec spec, std::time_t current = 0 );

private:
    static const std::time_t DAY;
    static void setMidnight( std::tm& tim );
    static std::tm getWeekStart( const std::tm& tim );
    static std::tm getMonthStart( const std::tm& tim );
    static std::tm getYearStart( const std::tm& tim );
};

};
