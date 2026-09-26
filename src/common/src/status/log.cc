#include "project/common/status/log.h"


namespace uni::common::status {

log to_log(std::string_view v) noexcept
{
    if (v == "info") return log::info;
    else if (v == "error") return log::error;
    else if (v == "warning") return log::warning;
    else if (v == "debug") return log::debug;
    else if (v == "trace") return log::trace;
    else if (v == "critical") return log::critical;
    else return log::unknown;
}


log to_log(int16_t v) noexcept
{
    switch(v) {
        case 1: return log::trace;
        case 2: return log::debug;
        case 3: return log::info;
        case 4: return log::warning;
        case 5: return log::error;
        case 6: return log::critical;
        default:
            return log::unknown;
    }
}


log to_log(http v) noexcept
{
    switch(static_cast<unsigned>(v) / 100) {
        case 1:     return log::info;
        case 2:     return log::info;
        case 3:     return log::info;
        case 4:     return log::warning;
        case 5:     return log::error;
        default:
            break;
    }
    return log::unknown;
}


log to_log(retCode v) noexcept
{
    switch(static_cast<unsigned>(v) / 10000) {
        case 1:     return log::info;
        case 2:     return log::info;
        case 3:     return log::info;
        case 4:     return log::warning;
        case 5:     return log::error;
        default:
            break;
    }
    return log::unknown;
}


std::string_view obsolete_reason(log v) 
{
    switch(static_cast<log>(v)) {
        case log::trace:                         return "TRACE";
        case log::debug:                         return "DEBUG";
        case log::info:                          return "INFO";
        case log::warning:                       return "WARN";
        case log::error:                         return "ERROR";
        case log::critical:                      return "CRIT";
        default:
            break;
    }
    return "<unknown-log>";
}


constexpr std::string_view log_color(log v)
{
    switch (static_cast<log>(v)) {
        case log::trace:    return "\033[90m";
        case log::debug:    return "\033[36m";
        case log::info:     return "\033[32m";
        case log::warning:  return "\033[33m";
        case log::error:    return "\033[31m";
        case log::critical: return "\033[1;31m";
        default:                   return "\033[0m";
    }
}


std::string colored_reason(log v)
{
    return std::string(log_color(v)) + std::string(obsolete_reason(v)) + std::string(log_reset);
}


bool important_log(log v) 
{
    switch (static_cast<log>(v)) {
        case log::info: 
        case log::trace:    
        case log::debug:    return false;
        default:
            return true;
    }
}

} // namespace uni::common::status