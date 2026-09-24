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



namespace uni {
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
        syslog(mess.what());
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
        syslog(msg.what());
    }
    return nullptr;
}


bool FileSink::log(const LogInfo& info) noexcept 
{
    std::string inf = format_(info);

    file_ << inf << '\n';
    if (file_.fail()) {
        syslog("Error writing to log file: ", strerror(errno));
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
        syslog("Failed to flush file: ", std::strerror(errno));
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

} // namespace logrr
} // namespace uni