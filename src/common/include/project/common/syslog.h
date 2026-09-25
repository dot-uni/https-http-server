#ifndef SYSLOG_INCLUDED
#define SYSLOG_INCLUDED

#include <source_location>
#include <string>
#include <string_view>

#include "project/common/log_level.h"


namespace uni {
namespace logrr {

struct ErrorMessage 
{
    std::string format;
    std::source_location loc;
    ErrorMessage(const char* f, std::source_location l = std::source_location::current()) : format(f), loc(l) {}
    ErrorMessage(std::string_view f, std::source_location l = std::source_location::current()) : format(f), loc(l) {}
};


template <typename... Args>
void syslog(std::FILE* out, log_level level, ErrorMessage&& em, Args&&... args) 
{
    try {
        std::string log_msg = "[" + std::string(obsolete_reason(level)) + "_SYSLOG] {}:({}:{}) " + em.format + "\n";
        fmt::print(out, fmt::runtime(log_msg), em.loc.file_name(), em.loc.line(), em.loc.column(), std::forward<Args>(args)...);
    } catch(const std::exception& msg) {
        std::cerr << msg.what() << '\n';
    }
}


template <typename... Args>
void sys_info(ErrorMessage em, Args&&... args) 
{
    syslog(stdout, log_level::info, std::move(em), std::forward<Args>(args)...);
}


template <typename... Args>
void sys_error(ErrorMessage em, Args&&... args) 
{
    syslog(stderr, log_level::error, std::move(em), std::forward<Args>(args)...);
}


template <typename... Args>
void sys_crit(ErrorMessage em, Args&&... args) 
{
    syslog(stderr, log_level::critical, std::move(em), std::forward<Args>(args)...);
}


template <typename... Args>
void sys_warn(ErrorMessage em, Args&&... args) 
{
    syslog(stdout, log_level::warning, std::move(em), std::forward<Args>(args)...);
}


} // namespace logrr
} // namespace uni


#endif