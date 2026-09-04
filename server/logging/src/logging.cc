#include "logging.h"


namespace detail {

std::string time_to_string(std::chrono::system_clock::time_point&& tp) 
{
    const auto sec = std::chrono::floor<std::chrono::seconds>(tp);
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(tp - sec).count();

    const std::time_t t = std::chrono::system_clock::to_time_t(sec);
    std::tm tm{};

    #ifdef _WIN32
        localtime_s(&tm, &t);
    #else
        localtime_r(&t, &tm);
    #endif

    return fmt::format("{:%Y-%m-%d %H:%M:%S}.{:03}", tm, ms);
}


void strerror(std::string_view msg) 
{
    std::cerr << __FILE_NAME__ << ":" << __LINE__ << " " << __func__ << R"( ")" << msg << R"(")" << '\n';
} 

} // namespace detail


namespace logrr {


/** logrr::SingleLineFormatter 
 */

std::string SingleLineFormatter::format(const LogRecord& r) noexcept
{
    std::string base = "";
    try {
        base = fmt::format("[{}] [{}] {}:({}:{}) `{}`", 
            r.timepoint, colored_reason(r.status), r.loc.file_name(), r.loc.line(), r.loc.column(), r.loc.function_name());
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

std::string JsonFormatter::format(const LogRecord& r) noexcept
{
    nlohmann::ordered_json j = {
        {"timepoint", std::move(r.timepoint)},
        {"status_code", r.status},
        {"status", logrr::obsolete_reason(r.status)},
        {"file", r.loc.file_name()},
        {"line", r.loc.line()},
        {"function", r.loc.function_name()},
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
    logrr::log_status status,
    std::vector<LogField>&& dtls,
    const std::source_location loc
) const noexcept 
{
    if (level_ <= status) {
        LogRecord record = {
            .status = status,
            .loc = loc,
            .timepoint = detail::time_to_string(std::chrono::system_clock::now()),
            .details = std::move(dtls)
        };
        std::for_each(sinks_.begin(), sinks_.end(), [&record](const auto& sink){
            sink->log(record);
        });
    }
}


void Logger::log(
    http::retCode code,
    std::vector<LogField>&& dtls,
    const std::source_location loc
) const noexcept
{
    log(http::to_log_status(code), std::move(dtls), loc);
}


void Logger::log(
    http::status status,
    std::vector<LogField>&& dtls,
    const std::source_location loc
) const noexcept
{
    log(http::to_log_status(status), std::move(dtls), loc);
}


// info
void Logger::info(std::vector<LogField>&& dtls, const std::source_location loc) const noexcept
{
    log(logrr::log_status::info, std::move(dtls), loc);
}


/// error
void Logger::error(std::vector<LogField>&& dtls, const std::source_location loc) const noexcept
{
    log(logrr::log_status::error, std::move(dtls), loc);
}


/// warning
void Logger::warning(std::vector<LogField>&& dtls, const std::source_location loc) const noexcept
{
    log(logrr::log_status::warning, std::move(dtls), loc);
}


/// critical
void Logger::critical(std::vector<LogField>&& dtls, const std::source_location loc) const noexcept
{
    log(logrr::log_status::critical, std::move(dtls), loc);
}


/// debug
void Logger::debug(std::vector<LogField>&& dtls, const std::source_location loc) const noexcept
{
    log(logrr::log_status::debug, std::move(dtls), loc);
}


/// trace
void Logger::trace(std::vector<LogField>&& dtls, const std::source_location loc) const noexcept
{
    log(logrr::log_status::trace, std::move(dtls), loc);
}


void Logger::flush() noexcept 
{
    std::for_each(sinks_.begin(), sinks_.end(), [](const auto& sink){
        sink->flush();
    });
}


void log_info(const Logger* const l, std::vector<LogField>&& dtls, const std::source_location loc) noexcept
{
    if (l) { l->info(std::move(dtls), loc); }
}

void log_error(const Logger* const l, std::vector<LogField>&& dtls, const std::source_location loc) noexcept
{
    if (l) { l->error(std::move(dtls), loc); }
}

void log_warning(const Logger* const l, std::vector<LogField>&& dtls, const std::source_location loc) noexcept
{
    if (l) { l->warning(std::move(dtls), loc); }
}

void log_critical(const Logger* const l, std::vector<LogField>&& dtls, const std::source_location loc) noexcept
{
    if (l) { l->critical(std::move(dtls), loc); }
}

void log_debug(const Logger* const l, std::vector<LogField>&& dtls, const std::source_location loc) noexcept
{
    if (l) { l->debug(std::move(dtls), loc); }
}

void log_trace(const Logger* const l, std::vector<LogField>&& dtls, const std::source_location loc) noexcept
{
    if (l) { l->trace(std::move(dtls), loc); }
}


void log_using_retcode(http::retCode code, const Logger* const l, std::vector<LogField>&& dtls, std::source_location loc) noexcept
{
    if (!l) return;

    http::status s = http::to_http_status(code);
    std::vector<LogField> dtls_base = {
        logrr::field("retCode", code),
        logrr::field("retMesg", http::retMesg(code)),
        logrr::field("status", s),
        logrr::field("obsolete_reason", http::obsolete_reason(s))
    };
    dtls_base.insert(dtls_base.end(), dtls.begin(), dtls.end());

    l->log(code, std::move(dtls_base), loc);
}


void log_using_status(http::status status, const Logger* const l, std::vector<LogField>&& dtls, std::source_location loc) noexcept
{
    if (!l) return;

    std::vector<LogField> dtls_base = {
        logrr::field("status", status),
        logrr::field("obsolete_reason", http::obsolete_reason(status))
    };
    dtls_base.insert(dtls_base.end(), dtls.begin(), dtls.end());

    l->log(status, std::move(dtls_base), loc);
}

} // namespace logrr