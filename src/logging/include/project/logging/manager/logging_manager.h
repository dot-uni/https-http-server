#ifndef LOGGING_MANAGER_INCLUDED
#define LOGGING_MANAGER_INCLUDED


#include <filesystem>
#include <optional>
#include <memory>
#include <yaml-cpp/yaml.h>

#include "project/logging/logging.h"
#include "project/common/status/log.h"
#include "project/logging/syslog.h"


namespace uni::logging::manager {

/**
 * LogManager is a special class whose responsibility is to manage the logger
 */

std::unique_ptr<ISink> CreateSink(const YAML::Node& sink);

class LogManager final
{
    std::unique_ptr<Logger> logger_;
    bool paused_;
public:
    LogManager(std::filesystem::path path);
    ~LogManager() = default;

    LogManager(const LogManager&) = delete;
    LogManager(LogManager&&) = default;

    LogManager& operator=(const LogManager&) = delete;
    LogManager& operator=(LogManager&&) = default;

    bool Init(std::filesystem::path path);
    void Pause() noexcept;
    void Remain() noexcept;
    void ShutDown() noexcept;

    std::optional<std::reference_wrapper<Logger>> Get() const noexcept;
    status::log GetLevel() const noexcept;
    bool Paused() const noexcept;
private:
    std::optional<LogConfig> ParseLogConfig(std::filesystem::path path);
}; 

} // namespace uni::logging::manager

#endif