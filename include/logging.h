#ifndef LOGGING_INCLUDED
#define LOGGING_INCLUDED


#include <vector>
#include <mutex>
#include <string>
#include <string_view>
#include <chrono>
#include <sstream>
#include <iostream>
#include <fstream>
#include <fmt/format.h>
#include <iomanip>
#include <nlohmann/json.hpp>

#include "tostring.h"
#include "log_status.h"


namespace {


std::string timeToString(std::chrono::system_clock::time_point tp) 
{
    std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm = *std::localtime(&t);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%F %T");
    return oss.str();
}


} // namespace


namespace logrr {

    
using LogField = std::pair<std::string, std::string>;

template <typename T>
LogField field(const std::string& key, T&& value) 
{
    return LogField{std::move(key), tostr::convertToString(std::forward<T>(value))};
}

struct LogRecord 
{
    logrr::log_status status;
    std::string_view func;      // __func__
    std::string_view file;      // __FILE_NAME__
    int line;
    std::string timepoint = timeToString(std::chrono::system_clock::now());
    std::vector<LogField> details = {};
    bool importance = true; 
};


struct ILogFormatter 
{
    virtual ~ILogFormatter() = default;
    virtual std::string format(const LogRecord&) const noexcept = 0;
};


class SingleLineFormatter : public ILogFormatter 
{
public:
    SingleLineFormatter() = default;
    virtual ~SingleLineFormatter() = default;
    std::string format(const LogRecord& r) const noexcept override;
};


struct JsonFormatter : public ILogFormatter 
{
public:
    JsonFormatter() = default;
    virtual ~JsonFormatter() = default;
    std::string format(const LogRecord& r) const noexcept override;
};


struct ILogSink 
{
    virtual ~ILogSink() = default;
    virtual bool log(const LogRecord& record) noexcept = 0;
    virtual bool flush() noexcept { return true; }
    virtual const char* name() const noexcept = 0;
};


class ConsoleSink final : public ILogSink 
{
public:
    ConsoleSink();
    ConsoleSink(std::shared_ptr<ILogFormatter> formatter);
    ~ConsoleSink() = default;
    bool log(const LogRecord& record) noexcept override;
    const char* name() const noexcept override { return "ConsoleSink"; }
private:
    std::mutex mtx_;
    std::shared_ptr<ILogFormatter> formatter_;
};


class FileSink final : public ILogSink 
{
public:
    FileSink();
    FileSink(const std::string& file_name);
    FileSink(const std::string& file_name, std::shared_ptr<ILogFormatter> formatter);
    ~FileSink() { file_.close(); }
    bool log(const LogRecord& record) noexcept override;
    bool flush() noexcept override;
    const char* name() const noexcept override { return "FileSink"; }
private:
    std::mutex mtx_;
    std::ofstream file_;
    std::shared_ptr<ILogFormatter> formatter_;
};


class Logger 
{
public:
    Logger() = default;
    Logger(const Logger&) noexcept;
    Logger(Logger&&) noexcept;
    virtual ~Logger() = default;
    Logger& operator=(const Logger&) noexcept;
    Logger& operator=(Logger&&) noexcept;

    bool addSink(std::shared_ptr<ILogSink> sink) noexcept;
    
    bool lInfo(const std::string& file, int line, const std::string& func, std::vector<LogField>&& dtls={}) noexcept;
    bool lError(const std::string& file, int line, const std::string& func, std::vector<LogField>&& dtls={}) noexcept;
    bool lWarning(const std::string& file, int line, const std::string& func, std::vector<LogField>&& dtls={}) noexcept;
    bool lCritical(const std::string& file, int line, const std::string& func, std::vector<LogField>&& dtls={}) noexcept;
    bool lDebug(const std::string& file, int line, const std::string& func, std::vector<LogField>&& dtls={}) noexcept;
    bool lTrace(const std::string& file, int line, const std::string& func, std::vector<LogField>&& dtls={}) noexcept;

    bool flush() noexcept;
protected:
    bool log(const LogRecord& record) noexcept;
    bool logTempl(
        logrr::log_status status,
        const std::string& file, 
        int line,
        const std::string& func, 
        std::vector<LogField>&& dtls
    ) noexcept;
protected:
    std::mutex mtx_;
    std::vector<std::shared_ptr<ILogSink>> sinks_;
};

} // namespace logrr

#endif