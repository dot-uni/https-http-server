#include "logging.h"


namespace detail {

std::string time_to_string(std::chrono::system_clock::time_point&& tp) 
{
    std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm = *std::localtime(&t);
    return fmt::format("{}", tm);
}


void strerror(const std::string& msg) 
{
    std::cerr << __FILE_NAME__ << ":" << __LINE__ << " " << __func__ << R"( ")" << msg << R"(")" << '\n';
} 

} // namespace detail


namespace logrr {


/** logrr::SingleLineFormatter 
 */

std::string SingleLineFormatter::format(LogRecord&& r) noexcept
{
    std::string base = "";
    try {
        base = fmt::format("{} [{}] {}:{} {}", 
            r.timepoint, colored_reason(r.status), r.file, r.line, r.func);
        for (auto&& detail : r.details) {
            base += fmt::format(R"( {}: "{}")", detail.first, detail.second);
        }
    }
    catch(fmt::format_error& mess) 
    {
        detail::strerror(mess.what());
    }
    return base;
}


/** logrr::JsonFormatter 
 */

std::string JsonFormatter::format(LogRecord&& r) noexcept
{
    nlohmann::ordered_json j = {
        {"timepoint", std::move(r.timepoint)},
        {"status_code", r.status},
        {"status", logrr::obsolete_reason(r.status)},
        {"file", r.file},
        {"line", r.line},
        {"function", r.func},
        {"details", std::move(r.details)}
    };
    return j.dump();
}


/** logrr::Logger 
 */

constexpr Logger::Logger(const Logger& logger) noexcept 
{
    sinks_ = logger.sinks_;
}

constexpr Logger::Logger(Logger&& logger) noexcept 
{
    sinks_ = std::move(logger.sinks_);
}

constexpr Logger& Logger::operator=(const Logger& logger) noexcept 
{
    if (&logger == this) return *this;
    sinks_ = logger.sinks_;
    return *this;
}

constexpr Logger& Logger::operator=(Logger&& logger) noexcept 
{
    sinks_ = std::move(logger.sinks_);
    return *this;
}


void Logger::log(
    log_status status,
    std::string_view file, 
    int line,
    std::string_view func,
    std::vector<LogField>&& dtls
) noexcept 
{
    if (level_ <= status) {
        LogRecord record = {
            .status = status,
            .file = file,
            .line = line,
            .func = func,
            .timepoint = detail::time_to_string(std::chrono::system_clock::now()),
            .details = std::move(dtls)
        };
        std::for_each(sinks_.begin(), sinks_.end(), [&record](const auto& sink){
            sink->log(record);
        });
    }
}


// info
void Logger::info(std::string_view file, int line, std::string_view func, std::vector<LogField>&& dtls) noexcept
{
    log(logrr::log_status::info, file, line, func, std::move(dtls));
}


/// error
void Logger::error(std::string_view file, int line, std::string_view func, std::vector<LogField>&& dtls) noexcept
{
    log(logrr::log_status::error, file, line, func, std::move(dtls));
}


/// warning
void Logger::warning(std::string_view file, int line, std::string_view func, std::vector<LogField>&& dtls) noexcept
{
    log(logrr::log_status::warning, file, line, func, std::move(dtls));
}


/// critical
void Logger::critical(std::string_view file, int line, std::string_view func, std::vector<LogField>&& dtls) noexcept
{
    log(logrr::log_status::critical, file, line, func, std::move(dtls));
}


/// debug
void Logger::debug(std::string_view file, int line, std::string_view func, std::vector<LogField>&& dtls) noexcept
{
    log(logrr::log_status::debug, file, line, func, std::move(dtls));
}


/// trace
void Logger::trace(std::string_view file, int line, std::string_view func, std::vector<LogField>&& dtls) noexcept
{
    log(logrr::log_status::trace, file, line, func, std::move(dtls));
}


void Logger::flush() noexcept 
{
    std::for_each(sinks_.begin(), sinks_.end(), [](const auto& sink){
        sink->flush();
    });
}

} // namespace logrr