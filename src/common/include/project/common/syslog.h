#ifndef SYSLOG_INCLUDED
#define SYSLOG_INCLUDED

#include <source_location>
#include <string>
#include <string_view>


namespace uni {
namespace logrr {

struct ErrorMessage 
{
    std::string_view msg;
    std::source_location loc;
    ErrorMessage(const char* m, std::source_location l = std::source_location::current()) : msg(m), loc(l) {}
    ErrorMessage(std::string_view m, std::source_location l = std::source_location::current()) : msg(m), loc(l) {}
};


template <typename... Args>
void syslog_to(std::FILE* out, ErrorMessage m, Args&&... args) 
{
    if (out == stderr) {
        fmt::print(out, R"(__[ERROR_SYSLOG]__: {}:({}:{}) {})", m.loc.file_name(), m.loc.line(), m.loc.column(), m.msg);
    } else {
        fmt::print(out, R"(__[INFO_SYSLOG]__: {}:({}:{}) {})", m.loc.file_name(), m.loc.line(), m.loc.column(), m.msg);
    }

    if constexpr (sizeof...(args) > 0) {
        (fmt::print(out, "{}", std::forward<Args>(args)), ...);
    }
    fmt::print(out, "\n");
}

template <typename... Args>
void syslog(ErrorMessage m, Args&&... args) 
{
    syslog_to(stderr, std::move(m), std::forward<Args>(args)...);
} 

} // namespace logrr
} // namespace uni


#endif