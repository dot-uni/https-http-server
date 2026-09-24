#ifndef LOGGING_SYSTEM_INCLUDED
#define LOGGING_SYSTEM_INCLUDED


#include <filesystem>
#include <memory>
#include <mutex>
#include <source_location>
#include <vector>
#include <string_view>

#include "project/logging/logging.h"
#include "project/logging/config_watcher.h"
#include "project/logging/logging_manager.h"
#include "project/common/log_level.h"
#include "project/common/status.h"
#include "project/common/ret_status.h"
#include "project/common/syslog.h"


#define LOG(level, ...)                                                            \
    for (bool should_log = logrr::ShouldLog(logrr::to_log_level(level)); should_log; should_log = false)     \
        if (auto logger = logrr::LogSystem::Get(); logger)                             \
            logrr::LogStream(*logger, level, std::source_location::current() __VA_OPT__(,) __VA_ARGS__)

#define LOG_INFO(...) LOG(logrr::log_level::info, __VA_ARGS__)
#define LOG_ERROR(...) LOG(logrr::log_level::error, __VA_ARGS__)
#define LOG_WARN(...) LOG(logrr::log_level::warning, __VA_ARGS__)
#define LOG_CRIT(...) LOG(logrr::log_level::critical, __VA_ARGS__)
#define LOG_DEBUG(...) LOG(logrr::log_level::debug, __VA_ARGS__)
#define LOG_TRACE(...) LOG(logrr::log_level::trace, __VA_ARGS__)

#define LOG_USING_RETCODE(code, ...) LOG(code __VA_OPT__(,) __VA_ARGS__)
#define LOG_USING_STATUS(status, ...) LOG(status __VA_OPT__(,) __VA_ARGS__)



namespace uni {
namespace logrr {

/**
 * For macros
 */

bool ShouldLog(logrr::log_level level) noexcept;

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



class LogSystem final
{
    inline static std::unique_ptr<LogManager> lm_ = nullptr;
    inline static std::unique_ptr<ConfigWatcher> cw_ = nullptr;
    inline static std::mutex mtx_;
public:
    static bool Init(std::filesystem::path path);

    static bool Start();
    static bool Start(std::filesystem::path path);
    static void Stop();

    static std::optional<std::reference_wrapper<Logger>> Get() noexcept;
    static log_level GetLevel() noexcept;
};

} // namespace logrr
} // namespace uni

#endif 
