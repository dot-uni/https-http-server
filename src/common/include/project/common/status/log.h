#ifndef LOG_STATUS_INCLUDED
#define LOG_STATUS_INCLUDED

#include <string>
#include <string_view>

#include "project/common/status/http.h"
#include "project/common/status/retcode.h"


namespace uni::common::status {

/**
 * Codes used for internal logging
 */

enum class log : int16_t 
{
    unknown = 0,
    trace,
    debug,
    info,
    warning,
    error,
    critical
};


constexpr log to_log(log v) noexcept { return v; }
log to_log(std::string_view v) noexcept;
log to_log(int16_t v) noexcept;
log to_log(http v) noexcept;
log to_log(retCode v) noexcept;

std::string_view obsolete_reason(log v);

constexpr std::string_view log_color(log v);

constexpr std::string_view log_reset = "\033[0m";

std::string colored_reason(log v);

bool important_log(log v);


} // namespace uni::common::status


#endif