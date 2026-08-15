#ifndef LOG_STATUS_INCLUDED
#define LOG_STATUS_INCLUDED

#include <string>
#include <string_view>

namespace logrr {

/**
 * Codes used for internal logging
 */

enum class log_status : int16_t 
{
    unknown = 0,
    trace = 100,
    debug = 101,
    info = 102,
    warning = 103,
    error = 104,
    critical = 105
};

std::string_view obsolete_reason(log_status v);

constexpr std::string_view log_color(log_status v);

constexpr std::string_view log_reset = "\033[0m";

std::string colored_reason(log_status v);

} // namespace logrr 

#endif