#ifndef LOGGING_INCLUDED
#define LOGGING_INCLUDED


#include <algorithm>
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


#define LOG_INFO(logger, ...)                                                  \
    do {                                                                       \
        static_assert(                                                         \
            logrr::is_logger_v<std::remove_cvref_t<decltype(*logger)>> ||      \
            logrr::is_slogger_v<std::remove_cvref_t<decltype(*logger)>>,       \
            "LOG_INFO: 'logger' must point to logrr::Logger or logrr::StatusLogger" \
        );                                                                     \
        auto&& logrr_logger_ = (logger);                                       \
        if (logrr_logger_ != nullptr) {                                        \
            logrr_logger_->info(__FILE_NAME__, __LINE__, __func__ __VA_OPT__(,) __VA_ARGS__); \
        }                                                                      \
    } while (false)


#define LOG_ERROR(logger, ...)                                                \
    do {                                                                       \
        static_assert(                                                         \
            logrr::is_logger_v<std::remove_cvref_t<decltype(*logger)>> ||      \
            logrr::is_slogger_v<std::remove_cvref_t<decltype(*logger)>>,       \
            "LOG_ERROR: 'logger' must point to logrr::Logger or logrr::StatusLogger" \
        );                                                                     \
        auto&& logrr_logger_ = (logger);                                      \
        if (logrr_logger_ != nullptr) {                                       \
            logrr_logger_->error(__FILE_NAME__, __LINE__, __func__ __VA_OPT__(,) __VA_ARGS__); \
        }                                                                      \
    } while (false)


#define LOG_WARN(logger, ...)                                                \
    do {                                                                       \
        static_assert(                                                         \
            logrr::is_logger_v<std::remove_cvref_t<decltype(*logger)>> ||      \
            logrr::is_slogger_v<std::remove_cvref_t<decltype(*logger)>>,       \
            "LOG_WARN: 'logger' must point to logrr::Logger or logrr::StatusLogger" \
        );                                                                     \
        auto&& logrr_logger_ = (logger);                                      \
        if (logrr_logger_ != nullptr) {                                       \
            logrr_logger_->warning(__FILE_NAME__, __LINE__, __func__ __VA_OPT__(,) __VA_ARGS__); \
        }                                                                      \
    } while (false)


#define LOG_CRIT(logger, ...)                                                \
    do {                                                                       \
        static_assert(                                                         \
            logrr::is_logger_v<std::remove_cvref_t<decltype(*logger)>> ||      \
            logrr::is_slogger_v<std::remove_cvref_t<decltype(*logger)>>,       \
            "LOG_CRIT: 'logger' must point to logrr::Logger or logrr::StatusLogger" \
        );                                                                     \
        auto&& logrr_logger_ = (logger);                                      \
        if (logrr_logger_ != nullptr) {                                       \
            logrr_logger_->critical(__FILE_NAME__, __LINE__, __func__ __VA_OPT__(,) __VA_ARGS__); \
        }                                                                      \
    } while (false)


#define LOG_DEBUG(logger, ...)                                                \
    do {                                                                       \
        static_assert(                                                         \
            logrr::is_logger_v<std::remove_cvref_t<decltype(*logger)>> ||      \
            logrr::is_slogger_v<std::remove_cvref_t<decltype(*logger)>>,       \
            "LOG_DEBUG: 'logger' must point to logrr::Logger or logrr::StatusLogger" \
        );                                                                     \
        auto&& logrr_logger_ = (logger);                                      \
        if (logrr_logger_ != nullptr) {                                       \
            logrr_logger_->debug(__FILE_NAME__, __LINE__, __func__ __VA_OPT__(,) __VA_ARGS__); \
        }                                                                      \
    } while (false)


#define LOG_TRACE(logger, ...)                                                \
    do {                                                                       \
        static_assert(                                                         \
            logrr::is_logger_v<std::remove_cvref_t<decltype(*logger)>> ||      \
            logrr::is_slogger_v<std::remove_cvref_t<decltype(*logger)>>,       \
            "LOG_TRACE: 'logger' must point to logrr::Logger or logrr::StatusLogger" \
        );                                                                     \
        auto&& logrr_logger_ = (logger);                                      \
        if (logrr_logger_ != nullptr) {                                       \
            logrr_logger_->trace(__FILE_NAME__, __LINE__, __func__ __VA_OPT__(,) __VA_ARGS__); \
        }                                                                      \
    } while (false)


namespace detail {

std::string time_to_string(std::chrono::system_clock::time_point&& tp);
void strerror(const std::string& msg);

} // namespace detail


namespace logrr {

class StatusLogger;
class Logger;

template <typename>
struct is_logger : std::false_type {};

template <>
struct is_logger<Logger> : std::true_type {};

template <typename T>
constexpr bool is_logger_v = is_logger<T>::value;


template <typename>
struct is_slogger : std::false_type {};

template <>
struct is_slogger<StatusLogger> : std::true_type {};

template <typename T>
constexpr bool is_slogger_v = is_slogger<T>::value;



struct LogRecord;

template <typename, typename=void>
struct has_format : std::false_type {};

template <typename T>
struct has_format<
    T,
    std::void_t<decltype(std::declval<T>().format(std::declval<LogRecord&&>()))>
> : std::is_same<
    decltype(std::declval<T>().format(std::declval<LogRecord&&>())),
    std::string
> {};

template <typename T>
constexpr bool has_format_v = has_format<T>::value;





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
    std::string_view func;      // __func__
    std::string_view file;      // __FILE_NAME__
    int line;
    std::string timepoint;
    std::vector<LogField> details = {};
};


struct SingleLineFormatter final
{
    static std::string format(LogRecord&& r) noexcept;
};


struct JsonFormatter final
{
    static std::string format(LogRecord&& r) noexcept;
};


struct ISink 
{
    virtual ~ISink() = default;
    virtual bool log(LogRecord& record) noexcept = 0;
    virtual bool flush() noexcept { return true; }
};


/** logrr::ConsoleSink 
 */

template <
    typename Formatter = SingleLineFormatter, 
    typename = std::enable_if_t<has_format_v<Formatter>>
> class ConsoleSink final : public ISink 
{
public:
    ConsoleSink() = default;
    bool log(LogRecord& record) noexcept override;
private:
    std::mutex mtx_;
};


template <typename Formatter, typename Enable>
bool ConsoleSink<Formatter, Enable>::log(LogRecord& record) noexcept 
{
    std::string inf;
    inf = Formatter::format(std::move(record));

    std::ostream& out = (important_log(record.status)) ? std::cerr : std::cout;
    out << inf << '\n';

    return static_cast<bool>(out);
}


/** logrr::FileSink 
 */


template <
    typename Formatter = JsonFormatter, 
    typename = std::enable_if_t<has_format_v<Formatter>>
> class FileSink final : public ISink 
{
public:
    FileSink();
    FileSink(std::string_view file_name);
    ~FileSink() { file_.close(); }
    bool log(LogRecord& record) noexcept override;
    bool flush() noexcept override;
private:
    std::mutex mtx_;
    std::ofstream file_;
};


template <typename Formatter, typename Enable>
FileSink<Formatter, Enable>::FileSink() : 
FileSink(fmt::format("log_{}.log", detail::time_to_string(std::chrono::system_clock::now()))) {}


template <typename Formatter, typename Enable>
FileSink<Formatter, Enable>::FileSink(std::string_view file_name) 
{
    file_.open(file_name, std::ios::app);
    if (!file_.is_open()) {
        throw std::runtime_error(fmt::format("{}:{} Failed to open file '{}': {}", 
                                    __FILE_NAME__, __LINE__, file_name, strerror(errno)));
    }
}


template <typename Formatter, typename Enable>
bool FileSink<Formatter, Enable>::log(LogRecord& record) noexcept 
{
    std::string inf;
    inf = Formatter::format(std::move(record));

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


template <typename Formatter, typename Enable>
bool FileSink<Formatter, Enable>::flush() noexcept 
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

class Logger 
{
public:
    constexpr Logger(logrr::log_status level = logrr::log_status::info) : level_(level) {}
    constexpr Logger(const Logger&) noexcept;
    constexpr Logger(Logger&&) noexcept;
    virtual ~Logger() = default;
    constexpr Logger& operator=(const Logger&) noexcept;
    constexpr Logger& operator=(Logger&&) noexcept;

    void info(std::string_view file, int line, std::string_view func, std::vector<LogField>&& dtls={}) noexcept;
    void error(std::string_view file, int line, std::string_view func, std::vector<LogField>&& dtls={}) noexcept;
    void warning(std::string_view file, int line, std::string_view func, std::vector<LogField>&& dtls={}) noexcept;
    void critical(std::string_view file, int line, std::string_view func, std::vector<LogField>&& dtls={}) noexcept;
    void debug(std::string_view file, int line, std::string_view func, std::vector<LogField>&& dtls={}) noexcept;
    void trace(std::string_view file, int line, std::string_view func, std::vector<LogField>&& dtls={}) noexcept;

    void flush() noexcept;

    template <typename Sink, typename... Args> void add_sink(Args&&... args);
    template <typename Sink> bool contain_sink() const noexcept;

    constexpr void set_log_level(logrr::log_status level) noexcept { level_ = level; }
protected:
    void log(
        log_status status,
        std::string_view file, 
        int line,
        std::string_view func,
        std::vector<LogField>&& dtls
    ) noexcept;
protected:
    logrr::log_status level_;
    std::vector<std::shared_ptr<ISink>> sinks_;
};


template <typename Sink, typename... Args> void Logger::add_sink(Args&&... args) 
{      
    static_assert(
        std::is_base_of_v<logrr::ISink, std::remove_cvref_t<Sink>>,
        "The passed type 'Sink' must be a subclass of 'ISink'"
    );
    if (contain_sink<Sink>()) {
        throw std::logic_error("Such a 'sink' already exists in std::vector<std::shared_ptr<ISink>> sinks_");
    }
    auto new_sink = std::make_shared<std::remove_cvref_t<Sink>>(args...);
    sinks_.push_back(new_sink);
}


template <typename Sink> bool Logger::contain_sink() const noexcept
{
    using target_type = std::remove_cvref_t<Sink>;
    static_assert(
        std::is_base_of_v<logrr::ISink, target_type>,
        "The passed type 'Sink' must be a subclass of 'ISink'"
    );

    return std::any_of(sinks_.begin(), sinks_.end(), [](const auto& sink) {
        return dynamic_cast<const target_type*>(sink.get()) != nullptr;
    });
}


} // namespace logrr

#endif