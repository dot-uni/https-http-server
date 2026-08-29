#include "logging.h"


namespace detail {

std::string time_to_string(std::chrono::system_clock::time_point&& tp) 
{
    std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm = *std::localtime(&t);
    return fmt::format("{}", tm);
}


void strerror(const std::string& msg) 
{
    std::cerr << __FILE_NAME__ << ":" << __LINE__ << " " << __func__ << R"( ")" << msg << R"(")" << '\n';
} 

} // namespace detail


namespace logrr {


/** logrr::SingleLineFormatter 
 */

std::string SingleLineFormatter::format(LogRecord&& r) const noexcept 
{
    std::string base = "";
    try {
        base = fmt::format("{} [{}] {}:{} {}", 
            r.timepoint, colored_reason(r.status), r.file, r.line, r.func);
        for (auto&& detail : r.details) {
            base += fmt::format(R"( {}: "{}")", detail.first, detail.second);
        }
    }
    catch(fmt::format_error& mess) 
    {
        detail::strerror(mess.what());
    }
    return base;
}


/** logrr::JsonFormatter 
 */

std::string JsonFormatter::format(LogRecord&& r) const noexcept 
{
    nlohmann::ordered_json j = {
        {"timepoint", std::move(r.timepoint)},
        {"status_code", r.status},
        {"status", logrr::obsolete_reason(r.status)},
        {"file", r.file},
        {"line", r.line},
        {"function", r.func},
        {"details", std::move(r.details)}
    };
    return j.dump();
}


/** logrr::ConsoleSink 
 */

ConsoleSink::ConsoleSink() : 
formatter_(std::make_shared<SingleLineFormatter>()) {}

ConsoleSink::ConsoleSink(std::shared_ptr<IFormatter>&& formatter) : 
formatter_(std::move(formatter)) {}

bool ConsoleSink::log(LogRecord& record) noexcept 
{
    std::string inf;
    inf = formatter_->format(std::move(record));

    std::ostream& out = (important_log(record.status)) ? std::cerr : std::cout;
    out << inf << '\n';

    return static_cast<bool>(out);
}


/** logrr::FileSink 
 */

FileSink::FileSink(std::string_view file_name, std::shared_ptr<IFormatter>&& formatter) : formatter_(std::move(formatter))
{
    file_.open(file_name, std::ios::app);
    if (!file_.is_open()) {
        throw std::runtime_error(fmt::format("{}:{} Failed to open file '{}': {}", 
                                    __FILE_NAME__, __LINE__, file_name, strerror(errno)));
    }
}

FileSink::FileSink(std::string_view file_name) :
FileSink(file_name, std::make_shared<JsonFormatter>()) {}

FileSink::FileSink() : 
FileSink(fmt::format("log_{}.log", detail::time_to_string(std::chrono::system_clock::now())), std::make_shared<JsonFormatter>()) {}

bool FileSink::log(LogRecord& record) noexcept 
{
    std::string inf;
    inf = formatter_->format(std::move(record));

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

bool FileSink::flush() noexcept 
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

constexpr Logger::Logger(const Logger& logger) noexcept 
{
    sinks_ = logger.sinks_;
}

constexpr Logger::Logger(Logger&& logger) noexcept 
{
    sinks_ = std::move(logger.sinks_);
}

constexpr Logger& Logger::operator=(const Logger& logger) noexcept 
{
    if (&logger == this) return *this;
    sinks_ = logger.sinks_;
    return *this;
}

constexpr Logger& Logger::operator=(Logger&& logger) noexcept 
{
    sinks_ = std::move(logger.sinks_);
    return *this;
}


void Logger::log(
    log_status status,
    std::string_view file, 
    int line,
    std::string_view func,
    std::vector<LogField>&& dtls
) noexcept 
{
    if (level_ <= status) {
        LogRecord record = {
            .status = status,
            .file = file,
            .line = line,
            .func = func,
            .timepoint = detail::time_to_string(std::chrono::system_clock::now()),
            .details = std::move(dtls)
        };
        std::for_each(sinks_.begin(), sinks_.end(), [&](const auto& sink){
            sink->log(record);
        });
    }
}


// info
void Logger::info(std::string_view file, int line, std::string_view func, std::vector<LogField>&& dtls) noexcept
{
    log(logrr::log_status::info, file, line, func, std::move(dtls));
}


/// error
void Logger::error(std::string_view file, int line, std::string_view func, std::vector<LogField>&& dtls) noexcept
{
    log(logrr::log_status::error, file, line, func, std::move(dtls));
}


/// warning
void Logger::warning(std::string_view file, int line, std::string_view func, std::vector<LogField>&& dtls) noexcept
{
    log(logrr::log_status::warning, file, line, func, std::move(dtls));
}


/// critical
void Logger::critical(std::string_view file, int line, std::string_view func, std::vector<LogField>&& dtls) noexcept
{
    log(logrr::log_status::critical, file, line, func, std::move(dtls));
}


/// debug
void Logger::debug(std::string_view file, int line, std::string_view func, std::vector<LogField>&& dtls) noexcept
{
    log(logrr::log_status::debug, file, line, func, std::move(dtls));
}


/// trace
void Logger::trace(std::string_view file, int line, std::string_view func, std::vector<LogField>&& dtls) noexcept
{
    log(logrr::log_status::trace, file, line, func, std::move(dtls));
}


void Logger::flush() noexcept 
{
    std::for_each(sinks_.begin(), sinks_.end(), [](const auto& sink){
        sink->flush();
    });
}

} // namespace logrr