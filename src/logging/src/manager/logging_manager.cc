#include "project/logging/manager/logging_manager.h"


namespace uni::logging::manager {

std::unique_ptr<ISink> CreateSink(const YAML::Node& sink)
{
    if (!sink.IsMap() || sink.size() != 1) {
        sys_error("Each sink entry must be a single-key mapping");
        return nullptr;
    }

    auto item = sink.begin();
    YAML::Node sink_name = item->first;
    YAML::Node settings = item->second;

    if (!sink_name.IsScalar()) {
        sys_error("The name 'sink' is not a scalar");
        return nullptr;
    }

    if (!settings.IsMap()) {
        sys_error("The set of arguments is not presented as a dictionary");
        return nullptr;
    }
    
    const auto enabled = settings["enabled"];

    if (!enabled || !enabled.IsScalar()) {
        sys_error("'enabled' is missing or is not a scalar");
        return nullptr;
    }

    if (!enabled.as<bool>()) {
        return nullptr;
    }

    std::string name = sink_name.as<std::string>();
    if (name == "console") return ConsoleSink::create(settings);
    else if (name == "file") return FileSink::create(settings);

    sys_error("There is no such sink: `{}`", name);
    return nullptr;

}


/** uni::logging::manager::LogManager 
 */

LogManager::LogManager(std::filesystem::path path) : logger_(nullptr), paused_(false)
{
    Init(std::move(path));
}


bool LogManager::Init(std::filesystem::path path)
{
    sys_info("Path to log config: '{}'", path.string());
    std::optional<LogConfig> config = ParseLogConfig(path);
    if (!config) { return false; }
    
    try {
        logger_ = std::make_unique<Logger>(std::move(*config));
    } catch(const std::exception& msg) {
        sys_error("The Logger was not created {}", msg.what());
        return false;
    }
    return true;
}


void LogManager::Pause() noexcept
{
    sys_info("A pause has been set in LogManager");
    paused_ = true;
}


void LogManager::Remain() noexcept
{
    sys_info("The pause in LogManager has been removed");
    paused_ = false;
}


void LogManager::ShutDown() noexcept
{
    logger_.reset();
}


std::optional<std::reference_wrapper<Logger>> LogManager::Get() const noexcept
{
    if (!logger_ || paused_) {
        return std::nullopt;
    }
    return std::ref(*logger_);
}


status::log LogManager::GetLevel() const noexcept
{
    if (!logger_) {
        return status::log::unknown;
    }
    return logger_->level();
}


bool LogManager::Paused() const noexcept
{
    return paused_;
}


std::optional<LogConfig> LogManager::ParseLogConfig(std::filesystem::path path)
{
    YAML::Node config;
    try {
        config = YAML::LoadFile(path.string());
    } catch(const YAML::Exception& msg) {
        sys_error(msg.what());
        return std::nullopt;
    }

    if (!config["logging"] || !config["logging"].IsMap()) {
        sys_error("'logging' is missing or is not a dictionary");
        return std::nullopt;
    }

    if (config["logging"]["enabled"]) {
        if (!config["logging"]["enabled"].IsScalar()) {
            sys_error("'enabled' field in 'logging' is not a scalar");
            return std::nullopt;
        }
        
        if (!config["logging"]["enabled"].as<bool>()) {
            Pause();
            sys_info("Logging has been disabled: 'enabled: false'");
            return std::nullopt;
        }
        else {
            Remain();
            sys_info("Logging has been enabled: 'enabled: true'");
        }
    }

    if (!config["logging"]["level"] || !config["logging"]["level"].IsScalar()) {
        sys_error("`level` is missing or is not a scalar");
        return std::nullopt;
    }

    if (!config["logging"]["sinks"] || !config["logging"]["sinks"].IsSequence()) {
        sys_error("`sinks` is missing or is not a sequence");
        return std::nullopt;
    } 

    status::log level = status::to_log(config["logging"]["level"].as<std::string>());
    if (level == status::log::unknown) {
        sys_error("unknown logging level");
        return std::nullopt;
    }

    std::vector<std::shared_ptr<ISink>> sinks;
    for (const auto& sink : config["logging"]["sinks"]) {
        if (auto created = CreateSink(sink)) {
            sinks.push_back(std::move(created));
        }
    }
    
    return LogConfig(level, std::move(sinks));
}

} // namespace uni::logging::manager