#ifndef LOG_LEVEL_INCLUDED
#define LOG_LEVEL_INCLUDED

#include <string>
#include <string_view>

#include "project/common/status.h"
#include "project/common/ret_status.h"


namespace logrr {

/**
 * Codes used for internal logging
 */

enum class log_level : int16_t 
{
    unknown = 0,
    trace,
    debug,
    info,
    warning,
    error,
    critical
};


constexpr log_level to_log_level(log_level v) noexcept { return v; }
log_level to_log_level(std::string_view v) noexcept;
log_level to_log_level(int16_t v) noexcept;
log_level to_log_level(http::status v) noexcept;
log_level to_log_level(http::retCode v) noexcept;

std::string_view obsolete_reason(log_level v);

constexpr std::string_view log_color(log_level v);

constexpr std::string_view log_reset = "\033[0m";

std::string colored_reason(log_level v);

bool important_log(log_level v);



} // namespace logrr 

#endif