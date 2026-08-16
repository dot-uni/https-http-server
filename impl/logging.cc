#include "logging.h"

namespace {

void strerror(const std::string& msg) 
{
    std::cerr << __FILE_NAME__ << ":" << __LINE__ << " " << __func__ << R"( ")" << msg << R"(")" << '\n';
} 

} // namespace


namespace logrr {


/** logrr::SingleLineFormatter 
 */

std::string SingleLineFormatter::format(const LogRecord& r) const noexcept 
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
        strerror(mess.what());
    }
    return base;
}


/** logrr::JsonFormatter 
 */

std::string JsonFormatter::format(const LogRecord& r) const noexcept 
{
    nlohmann::ordered_json j = {
        {"timepoint", r.timepoint},
        {"status_code", r.status},
        {"status", logrr::obsolete_reason(r.status)},
        {"file", r.file},
        {"line", r.line},
        {"function", r.func},
        {"details", r.details}
    };
    return j.dump();
}


/** logrr::ConsoleSink 
 */

ConsoleSink::ConsoleSink() : 
formatter_(std::make_shared<SingleLineFormatter>()) {}

ConsoleSink::ConsoleSink(std::shared_ptr<ILogFormatter> formatter) : 
formatter_(std::move(formatter)) {}

bool ConsoleSink::log(const LogRecord& record) noexcept 
{
    std::string inf;
    inf = formatter_->format(record);

    std::ostream& out = (record.importance) ? std::cerr : std::cout;
    out << inf << '\n';

    return static_cast<bool>(out);
}


/** logrr::FileSink 
 */

FileSink::FileSink(const std::string& file_name, std::shared_ptr<ILogFormatter> formatter) 
{
    file_.open(file_name, std::ios::app);
    if (!file_.is_open()) {
        throw std::runtime_error(fmt::format("{}:{} Failed to open file '{}': {}", 
                                    __FILE_NAME__, __LINE__, file_name, strerror(errno)));
    }
    formatter_ = std::move(formatter);
}

FileSink::FileSink(const std::string& file_name) :
FileSink(file_name, std::make_shared<JsonFormatter>()) {}

FileSink::FileSink() : 
FileSink(fmt::format("log_{}.log", timeToString(std::chrono::system_clock::now())), std::make_shared<JsonFormatter>()) {}

bool FileSink::log(const LogRecord& record) noexcept 
{
    std::string inf;
    inf = formatter_->format(record);

    file_ << inf << '\n';
    if (file_.fail()) {
        std::cerr << __FILE_NAME__ << ":" << __LINE__ << " " << "Error writing to log file: " << std::strerror(errno) << '\n';
        strerror(tostr::concat("Error writing to log file: ", std::strerror(errno)));
        file_.clear(); 
        return false;
    }

    if (record.importance) {
        return flush();
    }
    return true;
}

bool FileSink::flush() noexcept 
{
    file_.flush();
    if (file_.fail()) {
        strerror(tostr::concat("Failed to flush file: ", std::strerror(errno)));
        file_.clear(); 
        return false;
    }
    return true;
}


/** logrr::Logger  
 */

Logger::Logger(const Logger& logger) noexcept 
{
    sinks_ = logger.sinks_;
}

Logger::Logger(Logger&& logger) noexcept 
{
    sinks_ = std::move(logger.sinks_);
}

Logger& Logger::operator=(const Logger& logger) noexcept 
{
    if (&logger == this) return *this;
    sinks_ = logger.sinks_;
    return *this;
}

Logger& Logger::operator=(Logger&& logger) noexcept 
{
    sinks_ = std::move(logger.sinks_);
    return *this;
}


bool Logger::addSink(std::shared_ptr<ILogSink> sink) noexcept 
{
    if (!sink) return false;
    
    const char* sink_name = sink->name();
    for (auto&& existing : sinks_) {
        if (sink_name == existing->name()) {
            strerror(tostr::concat("The same sink already existing: ", sink_name));
            return false;
        }
    }
    try {
        sinks_.push_back(std::move(sink));
    } catch(std::bad_alloc& mess) {
        strerror(tostr::concat("Failed to add sink: ", sink_name, " - ", mess.what()));
        return false;
    }
    return true;
}

bool Logger::log(const LogRecord& record) noexcept 
{
    if (sinks_.empty()) {
        strerror("Sinks not added");
        return false;
    }

    bool success = true;
    for (auto&& sink : sinks_) {
        if (!sink->log(record)) {
            success = false;
        }
    }
    flush();

    return success;
}


// lInfo
bool Logger::lInfo(const std::string& file, int line, const std::string& func, std::vector<LogField>&& dtls) noexcept
{
    return log(LogRecord{
        .status = logrr::log_status::info,
        .file = file,
        .line = line,
        .func = func,
        .details = std::move(dtls)
    });
}


/// lError
bool Logger::lError(const std::string& file, int line, const std::string& func, std::vector<LogField>&& dtls) noexcept
{
    return log(LogRecord{
        .status = logrr::log_status::error,
        .file = file,
        .line = line,
        .func = func,
        .details = std::move(dtls)
    });
}


/// lWarning
bool Logger::lWarning(const std::string& file, int line, const std::string& func, std::vector<LogField>&& dtls) noexcept
{
    return log(LogRecord{
        .status = logrr::log_status::warning,
        .file = file,
        .line = line,
        .func = func,
        .details = std::move(dtls)
    });
}


/// lCritical
bool Logger::lCritical(const std::string& file, int line, const std::string& func, std::vector<LogField>&& dtls) noexcept
{
    return log(LogRecord{
        .status = logrr::log_status::critical,
        .file = file,
        .line = line,
        .func = func,
        .details = std::move(dtls)
    });
}


/// lDebug
bool Logger::lDebug(const std::string& file, int line, const std::string& func, std::vector<LogField>&& dtls) noexcept
{
    return log(LogRecord{
        .status = logrr::log_status::debug,
        .file = file,
        .line = line,
        .func = func,
        .details = std::move(dtls)
    });
}


/// lTrace
bool Logger::lTrace(const std::string& file, int line, const std::string& func, std::vector<LogField>&& dtls) noexcept
{
    return log(LogRecord{
        .status = logrr::log_status::trace,
        .file = file,
        .line = line,
        .func = func,
        .details = std::move(dtls)
    });
}


bool Logger::flush() noexcept 
{
    if (sinks_.empty()) {
        strerror("Sinks not added");
        return false;
    }

    bool success = true;
    for (auto&& sink : sinks_) {
        if (!sink->flush()) {
            success = false;
        }
    }
    return success;
}

} // namespace logrr