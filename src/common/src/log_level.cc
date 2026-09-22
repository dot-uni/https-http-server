#include "project/common/log_level.h"


namespace uni {
namespace logrr {

log_level to_log_level(std::string_view v) noexcept
{
    if (v == "info") return logrr::log_level::info;
    else if (v == "error") return logrr::log_level::error;
    else if (v == "warning") return logrr::log_level::warning;
    else if (v == "debug") return logrr::log_level::debug;
    else if (v == "trace") return logrr::log_level::trace;
    else if (v == "critical") return logrr::log_level::critical;
    else return logrr::log_level::unknown;
}


log_level to_log_level(int16_t v) noexcept
{
    switch(v) {
        case 1: return logrr::log_level::trace;
        case 2: return logrr::log_level::debug;
        case 3: return logrr::log_level::info;
        case 4: return logrr::log_level::warning;
        case 5: return logrr::log_level::error;
        case 6: return logrr::log_level::critical;
        default:
            return logrr::log_level::unknown;
    }
}


log_level to_log_level(http::status v) noexcept
{
    switch(static_cast<unsigned>(v) / 100) {
        case 1:     return logrr::log_level::info;
        case 2:     return logrr::log_level::info;
        case 3:     return logrr::log_level::info;
        case 4:     return logrr::log_level::warning;
        case 5:     return logrr::log_level::error;
        default:
            break;
    }
    return logrr::log_level::unknown;
}


log_level to_log_level(http::retCode v) noexcept
{
    switch(static_cast<unsigned>(v) / 10000) {
        case 1:     return logrr::log_level::info;
        case 2:     return logrr::log_level::info;
        case 3:     return logrr::log_level::info;
        case 4:     return logrr::log_level::warning;
        case 5:     return logrr::log_level::error;
        default:
            break;
    }
    return logrr::log_level::unknown;
}


std::string_view obsolete_reason(log_level v) 
{
    switch(static_cast<log_level>(v)) {
        case log_level::trace:                         return "TRACE";
        case log_level::debug:                         return "DEBUG";
        case log_level::info:                          return "INFO";
        case log_level::warning:                       return "WARN";
        case log_level::error:                         return "ERROR";
        case log_level::critical:                      return "CRIT";
        default:
            break;
    }
    return "<unknown-log_level>";
}


constexpr std::string_view log_color(log_level v)
{
    switch (static_cast<log_level>(v)) {
        case log_level::trace:    return "\033[90m";
        case log_level::debug:    return "\033[36m";
        case log_level::info:     return "\033[32m";
        case log_level::warning:  return "\033[33m";
        case log_level::error:    return "\033[31m";
        case log_level::critical: return "\033[1;31m";
        default:                   return "\033[0m";
    }
}


std::string colored_reason(log_level v)
{
    return std::string(log_color(v)) + std::string(obsolete_reason(v)) + std::string(log_reset);
}


bool important_log(log_level v) 
{
    switch (static_cast<log_level>(v)) {
        case log_level::info: 
        case log_level::trace:    
        case log_level::debug:    return false;
        default:
            return true;
    }
}

} // namespace logrr
} // namespace uni