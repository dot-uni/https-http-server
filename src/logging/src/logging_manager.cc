#include "project/logging/logging_manager.h"


namespace uni {
namespace logrr {

std::unique_ptr<ISink> CreateSink(const YAML::Node& sink)
{
    if (!sink.IsMap() || sink.size() != 1) {
        syslog("Each sink entry must be a single-key mapping");
        return nullptr;
    }

    auto item = sink.begin();
    YAML::Node sink_name = item->first;
    YAML::Node settings = item->second;

    if (!sink_name.IsScalar()) {
        syslog("The name 'sink' is not a scalar");
        return nullptr;
    }

    if (!settings.IsMap()) {
        syslog("The set of arguments is not presented as a dictionary");
        return nullptr;
    }
    
    const auto enabled = settings["enabled"];

    if (!enabled || !enabled.IsScalar()) {
        syslog("'enabled' is missing or is not a scalar");
        return nullptr;
    }

    if (!enabled.as<bool>()) {
        return nullptr;
    }

    std::string name = sink_name.as<std::string>();
    if (name == "console") return ConsoleSink::create(settings);
    else if (name == "file") return FileSink::create(settings);

    syslog("There is no such sink: `", name, "`");
    return nullptr;

}


/** logrr::LogManager 
 */

LogManager::LogManager(std::filesystem::path path) : logger_(nullptr), paused_(false)
{
    syslog_to(stdout, "Path to log config: '", path.string(), "'");
    std::optional<LogConfig> config = ParseLogConfig(path);
    if (!config) { return; }
    
    try {
        logger_ = std::make_unique<Logger>(std::move(*config));
    } catch(const std::exception& msg) {
        syslog("The Logger was not created", msg.what());
    }
}


bool LogManager::Init(std::filesystem::path path)
{
    syslog_to(stdout, "Path to log config: '", path.string(), "'");
    std::optional<LogConfig> config = ParseLogConfig(path);
    if (!config) { return false; }
    
    try {
        logger_ = std::make_unique<Logger>(std::move(*config));
    } catch(const std::exception& msg) {
        syslog("The Logger was not created", msg.what());
        return false;
    }
    return true;
}


void LogManager::Pause() noexcept
{
    syslog_to(stdout, "A pause has been set in LogManager");
    paused_ = true;
}


void LogManager::Remain() noexcept
{
    syslog_to(stdout, "The pause in LogManager has been removed");
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


log_level LogManager::GetLevel() const noexcept
{
    if (!logger_) {
        return log_level::unknown;
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
        syslog(msg.what());
        return std::nullopt;
    }

    if (!config["logging"] || !config["logging"].IsMap()) {
        syslog("'logging' is missing or is not a dictionary");
        return std::nullopt;
    }

    if (config["logging"]["enabled"]) {
        if (!config["logging"]["enabled"].IsScalar()) {
            syslog("'enabled' field in 'logging' is not a scalar");
            return std::nullopt;
        }
        
        if (!config["logging"]["enabled"].as<bool>()) {
            Pause();
            syslog_to(stdout, "Logging has been disabled: 'enabled: false'");
            return std::nullopt;
        }
        else {
            Remain();
            syslog_to(stdout, "Logging has been enabled: 'enabled: true'");
        }
    }

    if (!config["logging"]["level"] || !config["logging"]["level"].IsScalar()) {
        syslog("`level` is missing or is not a scalar");
        return std::nullopt;
    }

    if (!config["logging"]["sinks"] || !config["logging"]["sinks"].IsSequence()) {
        syslog("`sinks` is missing or is not a sequence");
        return std::nullopt;
    } 

    logrr::log_level level = logrr::to_log_level(config["logging"]["level"].as<std::string>());
    if (level == log_level::unknown) {
        syslog("unknown logging level");
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

} // namespace logrr
} // namespace uni