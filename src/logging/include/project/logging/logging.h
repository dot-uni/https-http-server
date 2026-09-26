#ifndef LOGGING_INCLUDED
#define LOGGING_INCLUDED


#include <algorithm>
#include <source_location>
#include <concepts>
#include <utility>
#include <string>
#include <string_view>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iostream>
#include <fstream>
#include <functional>
#include <fmt/args.h>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <fmt/format.h>
#include <fmt/chrono.h>
#include <fmt/ostream.h>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <yaml-cpp/yaml.h>

#include "project/common/tostring.h"
#include "project/common/status/log.h"
#include "project/common/status/http.h"
#include "project/common/status/retcode.h"
#include "project/logging/syslog.h"


namespace detail {

std::string time_to_string(std::chrono::system_clock::time_point&& tp);

} // namespace detail



namespace uni::logging {

namespace status = ::uni::common::status;

using LogField = std::pair<std::string, std::string>;


template <typename T>
constexpr LogField field(std::string_view key, T&& value) 
{
    using type = std::remove_cvref_t<T>;
    if constexpr (std::is_same_v<type, std::string>) {
        return LogField{key, std::forward<T>(value)};
    }
    else {
        return LogField{key, uni::common::to_string(std::forward<T>(value))};
    } 
}


struct LogInfo
{
    status::log status;
    std::string info;
    std::source_location loc;
    std::vector<LogField> details;
    std::string timepoint;

    LogInfo() = default;
    LogInfo(
        status::log s, 
        std::string_view i, 
        std::vector<LogField>&& d={},
        std::source_location l = std::source_location::current()
    ) : status(s), 
        info(i), 
        details(std::move(d)), 
        loc(std::move(l)), 
        timepoint(detail::time_to_string(std::chrono::system_clock::now())) {}
};


struct ISink 
{
    virtual ~ISink() = default;
    virtual bool log(const LogInfo&) noexcept = 0;
};


struct LogConfig
{
    status::log level;
    std::vector<std::shared_ptr<ISink>> sinks;
};


using Format = std::function<std::string(const LogInfo&)>;

std::string ConsoleFormat(const LogInfo&) noexcept;
std::string JsonFormat(const LogInfo&) noexcept;

Format FormatIs(std::string_view name) noexcept;



class ConsoleSink final : public ISink 
{
public:
    ConsoleSink(Format format = ConsoleFormat) : format_(format) {}
    static std::unique_ptr<ConsoleSink> create(const YAML::Node& config);
    bool log(const LogInfo& info) noexcept override;
private:
    Format format_;
};



class FileSink final : public ISink 
{
public:
    FileSink(Format format = JsonFormat);
    FileSink(std::string path, Format format);
    ~FileSink() { file_.close(); }

    static std::unique_ptr<FileSink> create(const YAML::Node& config);
    bool log(const LogInfo& info) noexcept override;
    bool flush() noexcept;
private:
    Format format_;
    std::ofstream file_;
};


class Logger 
{
public:
    Logger(const LogConfig& config);
    virtual ~Logger() = default;

    void log(const LogInfo& info) const noexcept;
    void log(
        status::log level, 
        std::string_view info, 
        std::vector<LogField>&& details={},
        std::source_location loc = std::source_location::current()
    ) const noexcept;

    constexpr void set_level(status::log level) noexcept { level_ = level; }
    constexpr status::log level() const noexcept { return level_; }
protected:
    status::log level_;
    std::vector<std::shared_ptr<ISink>> sinks_;
};

} // namespace uni::logging

#endif