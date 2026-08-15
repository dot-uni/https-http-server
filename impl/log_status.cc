#include "log_status.h"

namespace logrr {

std::string_view obsolete_reason(log_status v) 
{
    switch(static_cast<log_status>(v)) {
        case log_status::trace:                         return "TRACE";
        case log_status::debug:                         return "DEBUG";
        case log_status::info:                          return "INFO";
        case log_status::warning:                       return "WARN";
        case log_status::error:                         return "ERROR";
        case log_status::critical:                      return "CRIT";
        default:
            break;
    }
    return "<unknown-log_status>";
}


constexpr std::string_view log_color(log_status v)
{
    switch (v) {
        case log_status::trace:    return "\033[90m";
        case log_status::debug:    return "\033[36m";
        case log_status::info:     return "\033[32m";
        case log_status::warning:  return "\033[33m";
        case log_status::error:    return "\033[31m";
        case log_status::critical: return "\033[1;31m";
        default:                   return "\033[0m";
    }
}

std::string colored_reason(log_status v)
{
    return std::string(log_color(v)) + std::string(obsolete_reason(v)) + std::string(log_reset);
}

} // namespace logrr