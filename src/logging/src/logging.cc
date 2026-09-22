#include "project/logging/logging.h"


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

} // namespace detail


namespace logrr {


/** logrr::SingleLineFormatter 
 */

std::string ConsoleFormat(const LogInfo& i) noexcept
{
    std::string base = "";
    try {
        base = fmt::format(R"([{}] [{}] {}:({}:{}) "{}")", 
            i.timepoint, colored_reason(i.status), i.loc.file_name(), i.loc.line(), i.loc.column(), i.info);

        if (!i.details.empty()) {
            base += fmt::format(" details:\n");

            std::for_each(i.details.begin(), i.details.end(), [&base](const auto& detail){
                base += fmt::format("\t- {}: \"{}\"\n", detail.first, detail.second);
            });

            base.pop_back();
        }
    } catch(fmt::format_error& mess) {
        detail::print_error(mess.what());
    }
    return base;
}


/** logrr::JsonFormatter 
 */

std::string JsonFormat(const LogInfo& i) noexcept
{
    nlohmann::ordered_json j = {
        {"timepoint", std::move(i.timepoint)},
        {"status_code", i.status},
        {"status", logrr::obsolete_reason(i.status)},
        {"file", i.loc.file_name()},
        {"line", i.loc.line()},
        {"details", std::move(i.details)}
    };
    return j.dump();
}


Format FormatIs(std::string_view name) noexcept
{
    if (name == "console") return ConsoleFormat;
    else if (name == "json") return JsonFormat;
    return nullptr;
}


/** logrr::ConsoleSink 
 */

std::unique_ptr<ConsoleSink> ConsoleSink::create(const YAML::Node& config)
{
    if (!config || !config.IsMap()) return nullptr;
    
    const auto format_node = config["format"];
    if (!format_node || !format_node.IsScalar()) {
        return nullptr;
    }

    Format format = FormatIs(format_node.as<std::string>());
    if (!format) {
        return nullptr;
    }

    return std::make_unique<ConsoleSink>(format);
}


bool ConsoleSink::log(const LogInfo& info) noexcept 
{
    fmt::print("{}\n", format_(info));
    return true;
}


/** logrr::FileSink 
 */

FileSink::FileSink(Format format) : 
FileSink(fmt::format("log_{}.log", detail::time_to_string(std::chrono::system_clock::now())), format) {}


FileSink::FileSink(std::string path, Format format) : format_(format)
{
    if (!path.length()) {
        path = fmt::format("log_{}.log", detail::time_to_string(std::chrono::system_clock::now()));
    }

    file_.open(path, std::ios::app);
    if (!file_.is_open()) {
        throw std::runtime_error(fmt::format("Failed to open file `{}`: {}", path, strerror(errno)));
    }
}


std::unique_ptr<FileSink> FileSink::create(const YAML::Node& config)
{
    if (!config || !config.IsMap()) {
        return nullptr;
    }

    const auto format_node = config["format"];
    if (!format_node || !format_node.IsScalar()) {
        return nullptr;
    } 

    const auto path_node = config["path"];
    if (!path_node || !path_node.IsScalar()) {
        return nullptr;
    } 

    Format format = FormatIs(format_node.as<std::string>());
    if (!format) {
        return nullptr;
    }

    std::string path = path_node.as<std::string>();
    try {
        return std::make_unique<FileSink>(path, format);
    } catch(const std::exception& msg) {
        detail::print_error(msg.what());
    }
    return nullptr;
}


bool FileSink::log(const LogInfo& info) noexcept 
{
    std::string inf = format_(info);

    file_ << inf << '\n';
    if (file_.fail()) {
        detail::print_error("Error writing to log file: ", strerror(errno));
        file_.clear(); 
        return false;
    }

    if (important_log(info.status)) {
        return flush();
    }
    return true;
}


bool FileSink::flush() noexcept 
{
    file_.flush();
    if (file_.fail()) {
        detail::print_error("Failed to flush file: ", std::strerror(errno));
        file_.clear(); 
        return false;
    }
    return true;
}


/** logrr::Logger 
 */

Logger::Logger(const LogConfig& config) : level_(config.level), sinks_(std::move(config.sinks)) {}


void Logger::log(const LogInfo& info) const noexcept
{
    std::for_each(sinks_.begin(), sinks_.end(), [&info](const auto& sink){
        sink->log(info);
    });
}


void Logger::log(
    log_level level, 
    std::string_view info, 
    std::vector<LogField>&& details,
    std::source_location loc
) const noexcept { log(LogInfo{level, info, std::move(details), std::move(loc)}); }


/** logrr::LogStream 
 */

LogStream::LogStream(
    const Logger& logger, 
    log_level level, 
    std::source_location loc,
    std::string_view msg, 
    std::vector<LogField>&& details
) : logger_(logger), linfo_(level, msg, std::move(details), std::move(loc)) {}


LogStream::LogStream(
    const Logger& logger, 
    log_level level, 
    std::source_location loc,
    std::vector<LogField>&& details
) : LogStream(logger, level, std::move(loc), "", std::move(details)) {}


LogStream::LogStream(
    const Logger& logger, 
    http::retCode code, 
    std::source_location loc,
    std::string_view msg, 
    std::vector<LogField>&& details
) : logger_(logger)
{
    http::status s = http::to_http_status(code);
    std::vector<LogField> details_base = {
        logrr::field("retCode", code),
        logrr::field("retMesg", http::retMesg(code)),
        logrr::field("status", s),
        logrr::field("obsolete_reason", http::obsolete_reason(s))
    };
    details.insert(details.end(), details_base.begin(), details_base.end());

    linfo_ = LogInfo(to_log_level(code), msg, std::move(details), std::move(loc));
}


LogStream::LogStream(
    const Logger& logger, 
    http::retCode code, 
    std::source_location loc,
    std::vector<LogField>&& details
) : LogStream(logger, code, std::move(loc), "", std::move(details)) {}


LogStream::LogStream(
    const Logger& logger, 
    http::status status, 
    std::source_location loc,
    std::string_view msg, 
    std::vector<LogField>&& details
) : logger_(logger)
{
    std::vector<LogField> details_base = {
        logrr::field("status", status),
        logrr::field("obsolete_reason", http::obsolete_reason(status))
    };
    details.insert(details.end(), details_base.begin(), details_base.end());

    linfo_ = LogInfo(to_log_level(status), msg, std::move(details), std::move(loc));
}


LogStream::LogStream(
    const Logger& logger, 
    http::status status, 
    std::source_location loc,
    std::vector<LogField>&& details
) : LogStream(logger, status, std::move(loc), "", std::move(details)) {}


LogStream::~LogStream()
{
    try {
        if (!buffer_.empty()) {
            fmt::dynamic_format_arg_store<fmt::format_context> store;
            for (const auto& arg : buffer_) {
                store.push_back(arg);
            }
            linfo_.info = fmt::vformat(linfo_.info, store);
        }
        logger_.log(std::move(linfo_));
    } catch(const std::exception& msg) {
        detail::print_error(msg.what());
    } catch(...) {
        detail::print_error("unknown error in LogStream::~LogStream");
    }
} 


std::unique_ptr<ISink> CreateSink(const YAML::Node& sink)
{
    if (!sink.IsMap() || sink.size() != 1) {
        detail::print_error("Each sink entry must be a single-key mapping");
        return nullptr;
    }

    auto item = sink.begin();
    YAML::Node sink_name = item->first;
    YAML::Node settings = item->second;

    if (!sink_name.IsScalar()) {
        detail::print_error("The name 'sink' is not a scalar");
        return nullptr;
    }

    if (!settings.IsMap()) {
        detail::print_error("The set of arguments is not presented as a dictionary");
        return nullptr;
    }
    
    const auto enabled = settings["enabled"];

    if (!enabled || !enabled.IsScalar()) {
        detail::print_error("'enabled' is missing or is not a scalar");
        return nullptr;
    }

    if (!enabled.as<bool>()) {
        return nullptr;
    }

    std::string name = sink_name.as<std::string>();
    if (name == "console") return ConsoleSink::create(settings);
    else if (name == "file") return FileSink::create(settings);

    detail::print_error("There is no such sink: `", name, "`");
    return nullptr;

}


std::optional<LogConfig> ParseLogConfig(std::string_view config_name)
{
    YAML::Node config;
    try {
        config = YAML::LoadFile(std::string(config_name));
    } catch(const YAML::Exception& msg) {
        detail::print_error(msg.what());
        return std::nullopt;
    }

    if (!config["logging"] || !config["logging"].IsMap()) {
        detail::print_error("'logging' is missing or is not a dictionary");
        return std::nullopt;
    }

    if (!config["logging"]["level"] || !config["logging"]["level"].IsScalar()) {
        detail::print_error("`level` is missing or is not a scalar");
        return std::nullopt;
    }

    if (!config["logging"]["sinks"] || !config["logging"]["sinks"].IsSequence()) {
        detail::print_error("`sinks` is missing or is not a sequence");
        return std::nullopt;
    } 

    logrr::log_level level = logrr::to_log_level(config["logging"]["level"].as<std::string>());
    if (level == log_level::unknown) {
        detail::print_error("unknown logging level");
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


/** logrr::LogManager 
 */

bool LogManager::Init(std::string_view config_name)
{
    std::optional<LogConfig> config = ParseLogConfig(config_name);
    if (!config) { return false; }
    
    return Init(std::move(*config));
}


bool LogManager::Init(LogConfig&& config) 
{
    try {
        logger_ = std::make_unique<Logger>(std::move(config));
        return true;
    } catch(const std::exception& msg) {
        detail::print_error(msg.what());
        return false;
    }
}


std::optional<std::reference_wrapper<Logger>> LogManager::Get() noexcept
{
    if (!logger_) {
        return std::nullopt;
    }
    return std::ref(*logger_);
}


log_level LogManager::GetLevel() noexcept
{
    if (!logger_) {
        return log_level::unknown;
    }
    return logger_->level();
}


void LogManager::ShutDown() noexcept
{
    logger_.reset();
}


bool ShouldLog(logrr::log_level level) noexcept
{
    log_level set_level = LogManager::GetLevel();

    if (!static_cast<int>(set_level) || set_level > level) {
        return false;
    }
    return true;
}

} // namespace logrr