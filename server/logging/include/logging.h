#ifndef LOGGING_INCLUDED
#define LOGGING_INCLUDED


#include <algorithm>
#include <source_location>
#include <concepts>
#include <utility>
#include <string>
#include <string_view>
#include <chrono>
#include <sstream>
#include <iostream>
#include <fstream>
#include <fmt/format.h>
#include <fmt/chrono.h>
#include <iomanip>
#include <nlohmann/json.hpp>

#include "tostring.h"
#include "log_status.h"
#include "status.h"
#include "ret_status.h"


namespace detail {

std::string time_to_string(std::chrono::system_clock::time_point&& tp);
void strerror(const std::string& msg);

} // namespace detail


namespace logrr {

struct LogRecord;

template <typename Formatter>
concept HasFormat = requires(LogRecord l) {
    { Formatter::format(std::move(l)) } -> std::convertible_to<std::string>;
};


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


struct LogRecord 
{
    logrr::log_status status;
    std::source_location loc;
    std::string timepoint;
    std::vector<LogField> details = {};
};


struct SingleLineFormatter final
{
    static std::string format(const LogRecord& r) noexcept;
};


struct JsonFormatter final
{
    static std::string format(const LogRecord& r) noexcept;
};


struct ISink 
{
    virtual ~ISink() = default;
    virtual bool log(const LogRecord& record) noexcept = 0;
    virtual bool flush() noexcept { return true; }
};


/** logrr::ConsoleSink 
 */

template <typename Formatter = SingleLineFormatter> 
requires HasFormat<Formatter>
class ConsoleSink final : public ISink 
{
public:
    ConsoleSink() = default;
    bool log(const LogRecord& record) noexcept override;
private:
    std::mutex mtx_;
};


template <typename Formatter>
requires HasFormat<Formatter>
bool ConsoleSink<Formatter>::log(const LogRecord& record) noexcept 
{
    std::string inf;
    inf = Formatter::format(record);

    std::ostream& out = (important_log(record.status)) ? std::cerr : std::cout;
    out << inf << '\n';

    return static_cast<bool>(out);
}


/** logrr::FileSink 
 */


template <typename Formatter = JsonFormatter> 
requires HasFormat<Formatter>
class FileSink final : public ISink 
{
public:
    FileSink();
    FileSink(std::string_view file_name);
    ~FileSink() { file_.close(); }
    bool log(const LogRecord& record) noexcept override;
    bool flush() noexcept override;
private:
    std::mutex mtx_;
    std::ofstream file_;
};


template <typename Formatter>
requires HasFormat<Formatter>
FileSink<Formatter>::FileSink() : 
FileSink(fmt::format("log_{}.log", detail::time_to_string(std::chrono::system_clock::now()))) {}


template <typename Formatter>
requires HasFormat<Formatter>
FileSink<Formatter>::FileSink(std::string_view file_name) 
{
    file_.open(file_name, std::ios::app);
    if (!file_.is_open()) {
        throw std::runtime_error(fmt::format("{}:{} Failed to open file '{}': {}", 
                                    __FILE_NAME__, __LINE__, file_name, strerror(errno)));
    }
}


template <typename Formatter>
requires HasFormat<Formatter>
bool FileSink<Formatter>::log(const LogRecord& record) noexcept 
{
    std::string inf;
    inf = Formatter::format(record);

    file_ << inf << '\n';
    if (file_.fail()) {
        std::cerr << __FILE_NAME__ << ":" << __LINE__ << " " << "Error writing to log file: " << std::strerror(errno) << '\n';
        detail::strerror(frmt::concat("Error writing to log file: ", std::strerror(errno)));
        file_.clear(); 
        return false;
    }

    if (important_log(record.status)) {
        return flush();
    }
    return true;
}


template <typename Formatter>
requires HasFormat<Formatter>
bool FileSink<Formatter>::flush() noexcept 
{
    file_.flush();
    if (file_.fail()) {
        detail::strerror(frmt::concat("Failed to flush file: ", std::strerror(errno)));
        file_.clear(); 
        return false;
    }
    return true;
}


/** logrr::Logger 
 */

template <typename Sink>
concept IsSink = std::derived_from<Sink, logrr::ISink>;


class Logger 
{
public:
    constexpr Logger(logrr::log_status level = logrr::log_status::info) : level_(level) {}
    constexpr Logger(const Logger&) noexcept;
    constexpr Logger(Logger&&) noexcept;
    virtual ~Logger() = default;
    constexpr Logger& operator=(const Logger&) noexcept;
    constexpr Logger& operator=(Logger&&) noexcept;

    void log(
        logrr::log_status status,
        std::vector<LogField>&& dtls={},
        const std::source_location loc = std::source_location::current()
    ) const noexcept;

    void log(
        http::retCode code,
        std::vector<LogField>&& dtls={},
        const std::source_location loc = std::source_location::current()
    ) const noexcept;

    void log(
        http::status status,
        std::vector<LogField>&& dtls={},
        const std::source_location loc = std::source_location::current()
    ) const noexcept;

    void info(std::vector<LogField>&& dtls={}, const std::source_location loc = std::source_location::current()) const noexcept;
    void error(std::vector<LogField>&& dtls={}, const std::source_location loc = std::source_location::current()) const noexcept;
    void warning(std::vector<LogField>&& dtls={}, const std::source_location loc = std::source_location::current()) const noexcept;
    void critical(std::vector<LogField>&& dtls={}, const std::source_location loc = std::source_location::current()) const noexcept;
    void debug(std::vector<LogField>&& dtls={}, const std::source_location loc = std::source_location::current()) const noexcept;
    void trace(std::vector<LogField>&& dtls={}, const std::source_location loc = std::source_location::current()) const noexcept;

    void flush() noexcept;

    template <typename Sink, typename... Args> 
    requires IsSink<Sink> void add_sink(Args&&... args);

    template <typename Sink> 
    requires IsSink<Sink> bool contain_sink() const noexcept;

    constexpr void set_level(logrr::log_status level) noexcept { level_ = level; }
    constexpr logrr::log_status level() const noexcept { return level_; }
protected:
    logrr::log_status level_;
    std::vector<std::shared_ptr<ISink>> sinks_;
};


template <typename Sink, typename... Args> 
requires IsSink<Sink> 
void Logger::add_sink(Args&&... args) 
{      
    if (contain_sink<Sink>()) {
        throw std::logic_error("Such a 'sink' already exists in std::vector<std::shared_ptr<ISink>> sinks_");
    }
    auto new_sink = std::make_shared<std::remove_cvref_t<Sink>>(std::forward<Args>(args)...);
    sinks_.push_back(new_sink);
}


template <typename Sink> 
requires IsSink<Sink>
bool Logger::contain_sink() const noexcept 
{
    using target_type = std::remove_cvref_t<Sink>;
    return std::any_of(sinks_.begin(), sinks_.end(), [](const auto& sink) {
        return dynamic_cast<const target_type*>(sink.get()) != nullptr;
    });
}


void log_info(const Logger* const l, std::vector<LogField>&& dtls={}, const std::source_location loc = std::source_location::current()) noexcept;
void log_error(const Logger* const l, std::vector<LogField>&& dtls={}, const std::source_location loc = std::source_location::current()) noexcept;
void log_warning(const Logger* const l, std::vector<LogField>&& dtls={}, const std::source_location loc = std::source_location::current()) noexcept;
void log_critical(const Logger* const l, std::vector<LogField>&& dtls={}, const std::source_location loc = std::source_location::current()) noexcept;
void log_debug(const Logger* const l, std::vector<LogField>&& dtls={}, const std::source_location loc = std::source_location::current()) noexcept;
void log_trace(const Logger* const l, std::vector<LogField>&& dtls={}, const std::source_location loc = std::source_location::current()) noexcept;

void log_using_retcode(http::retCode code, const Logger* const l, std::vector<LogField>&& dtls={}, std::source_location loc = std::source_location::current()) noexcept;
void log_using_status(http::status status, const Logger* const l, std::vector<LogField>&& dtls={}, std::source_location loc = std::source_location::current()) noexcept;


} // namespace logrr

#endif