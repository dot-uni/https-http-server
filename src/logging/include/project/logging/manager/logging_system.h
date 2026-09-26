#ifndef LOGGING_SYSTEM_INCLUDED
#define LOGGING_SYSTEM_INCLUDED


#include <filesystem>
#include <memory>
#include <mutex>
#include <source_location>
#include <vector>
#include <string_view>

#include "project/logging/logging.h"
#include "project/logging/config/configurator.h"
#include "project/logging/manager/logging_manager.h"
#include "project/common/status/log.h"
#include "project/common/status/http.h"
#include "project/common/status/retcode.h"
#include "project/logging/syslog.h"


#define LOG(level, ...)                                                            \
    for (bool should_log = uni::logging::manager::ShouldLog(uni::common::status::to_log(level)); should_log; should_log = false)     \
        if (auto logger = uni::logging::manager::LogSystem::Get(); logger)                             \
            uni::logging::manager::LogStream(*logger, level, std::source_location::current() __VA_OPT__(,) __VA_ARGS__)

#define LOG_INFO(...) LOG(uni::common::status::log::info, __VA_ARGS__)
#define LOG_ERROR(...) LOG(uni::common::status::log::error, __VA_ARGS__)
#define LOG_WARN(...) LOG(uni::common::status::log::warning, __VA_ARGS__)
#define LOG_CRIT(...) LOG(uni::common::status::log::critical, __VA_ARGS__)
#define LOG_DEBUG(...) LOG(uni::common::status::log::debug, __VA_ARGS__)
#define LOG_TRACE(...) LOG(uni::common::status::log::trace, __VA_ARGS__)

#define LOG_USING_RETCODE(code, ...) LOG(code __VA_OPT__(,) __VA_ARGS__)
#define LOG_USING_STATUS(status, ...) LOG(status __VA_OPT__(,) __VA_ARGS__)



namespace uni::logging::manager {

/**
 * For macros
 */

bool ShouldLog(status::log level) noexcept;

class LogStream final
{
public:
    // status::log 
    LogStream(
        const Logger& logger, 
        status::log level, 
        std::source_location loc,
        std::string_view msg, 
        std::vector<LogField>&& details={}
    );
    LogStream(
        const Logger& logger, 
        status::log level, 
        std::source_location loc,
        std::vector<LogField>&& details
    );

    // status::retCode 
    LogStream(
        const Logger& logger, 
        status::retCode code, 
        std::source_location loc,
        std::string_view msg, 
        std::vector<LogField>&& details={}
    );
    LogStream(
        const Logger& logger, 
        status::retCode code, 
        std::source_location loc,
        std::vector<LogField>&& details={}
    );

    // status::http 
    LogStream(
        const Logger& logger, 
        status::http status, 
        std::source_location loc,
        std::string_view msg, 
        std::vector<LogField>&& details={}
    );
    LogStream(
        const Logger& logger, 
        status::http status, 
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
    buffer_.push_back(uni::common::to_string(v));
    return *this;
}



class LogSystem final
{
    inline static std::unique_ptr<LogManager> lm_ = nullptr;
    inline static std::unique_ptr<config::ConfigWatcher> cw_ = nullptr;
    inline static std::mutex mtx_;
public:
    static bool Init(std::filesystem::path path);

    static bool Start();
    static bool Start(std::filesystem::path path);
    static void Stop();

    static std::optional<std::reference_wrapper<Logger>> Get() noexcept;
    static status::log GetLevel() noexcept;
};

} // namespace uni::logging::manager

#endif 
