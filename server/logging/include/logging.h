#ifndef LOGGING_INCLUDED
#define LOGGING_INCLUDED


#include <algorithm>
#include <source_location>
#include <concepts>
#include <utility>
#include <string>
#include <string_view>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iostream>
#include <fstream>
#include <functional>
#include <fmt/args.h>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <fmt/format.h>
#include <fmt/chrono.h>
#include <fmt/ostream.h>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <yaml-cpp/yaml.h>

#include "tostring.h"
#include "log_level.h"
#include "status.h"
#include "ret_status.h"


#define LOG(level, ...)                                                            \
    for (bool should_log = logrr::ShouldLog(logrr::to_log_level(level)); should_log; should_log = false)     \
        if (auto logger = logrr::LogManager::Get(); logger)                             \
            logrr::LogStream(*logger, level, std::source_location::current() __VA_OPT__(,) __VA_ARGS__)

#define LOG_INFO(...) LOG(logrr::log_level::info, __VA_ARGS__)
#define LOG_ERROR(...) LOG(logrr::log_level::error, __VA_ARGS__)
#define LOG_WARN(...) LOG(logrr::log_level::warning, __VA_ARGS__)
#define LOG_CRIT(...) LOG(logrr::log_level::critical, __VA_ARGS__)
#define LOG_DEBUG(...) LOG(logrr::log_level::debug, __VA_ARGS__)
#define LOG_TRACE(...) LOG(logrr::log_level::trace, __VA_ARGS__)

#define LOG_USING_RETCODE(code, ...) LOG(code __VA_OPT__(,) __VA_ARGS__)
#define LOG_USING_STATUS(status, ...) LOG(status __VA_OPT__(,) __VA_ARGS__)


namespace detail {

std::string time_to_string(std::chrono::system_clock::time_point&& tp);


struct ErrorMessage 
{
    std::string_view msg;
    std::source_location loc;
    ErrorMessage(const char* m, std::source_location l = std::source_location::current()) : msg(m), loc(l) {}
    ErrorMessage(std::string_view m, std::source_location l = std::source_location::current()) : msg(m), loc(l) {}
};


template <typename... Args>
void print_error(ErrorMessage m, Args&&... args) 
{
    fmt::print(stderr, R"([error]: {}:({}:{}) {})", m.loc.file_name(), m.loc.line(), m.loc.column(), m.msg);
    if (!sizeof...(args)) {
        (fmt::print(stderr, "{}", args), ...);
    }
    fmt::print(stderr, "\n");
} 

} // namespace detail


namespace logrr {

using LogField = std::pair<std::string, std::string>;


template <typename T>
constexpr LogField field(std::string_view key, T&& value) 
{
    using type = std::remove_cvref_t<T>;
    if constexpr (std::is_same_v<type, std::string>) {
        return LogField{key, std::forward<T>(value)};
    }
    else {
        return LogField{key, frmt::to_string(std::forward<T>(value))};
    } 
}


struct LogInfo
{
    log_level status;
    std::string info;
    std::source_location loc;
    std::vector<LogField> details;
    std::string timepoint;

    LogInfo() = default;
    LogInfo(
        log_level s, 
        std::string_view i, 
        std::vector<LogField>&& d={},
        std::source_location l = std::source_location::current()
    ) : status(s), 
        info(i), 
        details(std::move(d)), 
        loc(std::move(l)), 
        timepoint(detail::time_to_string(std::chrono::system_clock::now())) {}
};


struct ISink 
{
    virtual ~ISink() = default;
    virtual bool log(const LogInfo&) noexcept = 0;
};


struct LogConfig
{
    logrr::log_level level;
    std::vector<std::shared_ptr<ISink>> sinks;
};


using Format = std::function<std::string(const LogInfo&)>;

std::string ConsoleFormat(const LogInfo&) noexcept;
std::string JsonFormat(const LogInfo&) noexcept;

Format FormatIs(std::string_view name) noexcept;



class ConsoleSink final : public ISink 
{
public:
    ConsoleSink(Format format = ConsoleFormat) : format_(format) {}
    static std::unique_ptr<ConsoleSink> create(const YAML::Node& config);
    bool log(const LogInfo& info) noexcept override;
private:
    Format format_;
};



class FileSink final : public ISink 
{
public:
    FileSink(Format format = JsonFormat);
    FileSink(std::string path, Format format);
    ~FileSink() { file_.close(); }

    static std::unique_ptr<FileSink> create(const YAML::Node& config);
    bool log(const LogInfo& info) noexcept override;
    bool flush() noexcept;
private:
    Format format_;
    std::ofstream file_;
};



template <typename Sink>
concept IsSink = std::derived_from<Sink, logrr::ISink>;


class Logger 
{
public:
    Logger(const LogConfig& config);
    virtual ~Logger() = default;

    void log(const LogInfo& info) const noexcept;
    void log(
        log_level level, 
        std::string_view info, 
        std::vector<LogField>&& details={},
        std::source_location loc = std::source_location::current()
    ) const noexcept;

    constexpr void set_level(logrr::log_level level) noexcept { level_ = level; }
    constexpr logrr::log_level level() const noexcept { return level_; }
protected:
    log_level level_;
    std::vector<std::shared_ptr<ISink>> sinks_;
};



class LogStream final
{
public:
    // logrr::log_level 
    LogStream(
        const Logger& logger, 
        log_level level, 
        std::source_location loc,
        std::string_view msg, 
        std::vector<LogField>&& details={}
    );
    LogStream(
        const Logger& logger, 
        log_level level, 
        std::source_location loc,
        std::vector<LogField>&& details
    );

    // http::retCode 
    LogStream(
        const Logger& logger, 
        http::retCode code, 
        std::source_location loc,
        std::string_view msg, 
        std::vector<LogField>&& details={}
    );
    LogStream(
        const Logger& logger, 
        http::retCode code, 
        std::source_location loc,
        std::vector<LogField>&& details={}
    );

    // http::status 
    LogStream(
        const Logger& logger, 
        http::status status, 
        std::source_location loc,
        std::string_view msg, 
        std::vector<LogField>&& details={}
    );
    LogStream(
        const Logger& logger, 
        http::status status, 
        std::source_location loc,
        std::vector<LogField>&& details={}
    );

    ~LogStream();
    template <typename T> LogStream& operator<<(const T& v);
private:
    Logger logger_;
    LogInfo linfo_;
    std::vector<std::string> buffer_;
};


template <typename T> 
LogStream& LogStream::operator<<(const T& v)
{
    buffer_.push_back(frmt::to_string(v));
    return *this;
}



std::unique_ptr<ISink> CreateSink(const YAML::Node& sink);
std::optional<LogConfig> ParseLogConfig(std::string_view config_name);



class LogManager final
{
public:
    static bool Init(std::string_view config_name);
    static bool Init(LogConfig&& config);
    static std::optional<std::reference_wrapper<Logger>> Get() noexcept;
    static log_level GetLevel() noexcept;
    static void ShutDown() noexcept;
private:
    inline static std::unique_ptr<Logger> logger_ = nullptr;
}; 


bool ShouldLog(logrr::log_level level) noexcept;

} // namespace logrr

#endif