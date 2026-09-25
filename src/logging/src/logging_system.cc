#include "project/logging/logging_system.h"


namespace uni {
namespace logrr {

bool ShouldLog(logrr::log_level level) noexcept
{
    log_level set_level = LogSystem::GetLevel();

    if (!static_cast<int>(set_level) || set_level > level) {
        return false;
    }
    return true;
}


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
        sys_error(msg.what());
    } catch(...) {
        sys_error("unknown error in LogStream::~LogStream");
    }
} 



bool LogSystem::Init(std::filesystem::path path)
{
    std::unique_lock lock(mtx_);

    try {
        lm_ = std::make_unique<LogManager>(path);
        cw_ = std::make_unique<ConfigWatcher>(
            path, 
            [](std::filesystem::path p) { return lm_->Init(p); }
        );
    } catch(const std::bad_alloc& msg) {
        sys_error("Error initializing LogManager and ConfigWatcher: {}", msg.what());
        return false;
    } catch(const std::runtime_error& msg) {
        sys_error("Error in the constructor: {}", msg.what());
        return false;
    }

    lm_->Pause();

    lock.unlock();

    sys_info("LogManager and ConfigWatcher initialized");
    return true;
}


bool LogSystem::Start() 
{
    std::lock_guard lock(mtx_);
    if (!lm_ || !cw_) {
        sys_error("The LogSystem::Init method was not called to initialize the linking");
        return false;
    }

    cw_->Start();
    if (lm_->Paused()) {
        lm_->Remain();
    }

    return true;
}


bool LogSystem::Start(std::filesystem::path path)
{
    std::unique_lock lock(mtx_);

    try {
        lm_ = std::make_unique<LogManager>(path);
        cw_ = std::make_unique<ConfigWatcher>(
            path, 
            [](std::filesystem::path p) { return lm_->Init(p); }
        );
    } catch(const std::bad_alloc& msg) {
        sys_error("Error initializing LogManager and ConfigWatcher: {}", msg.what());
        return false;
    } catch(const std::runtime_error& msg) {
        sys_error("Error in the constructor: {}", msg.what());
        return false;
    }

    cw_->Start();

    lock.unlock();

    sys_info("The LogSystem was successfully started");
    return true;
}


void LogSystem::Stop()
{
    std::lock_guard lock(mtx_);
    
    if (!lm_ || !cw_) {
        sys_error("The LogSystem::Init method was not called to initialize the linking");
        return;
    }

    cw_->Stop();
    lm_->Pause();
}


std::optional<std::reference_wrapper<Logger>> LogSystem::Get() noexcept
{
    if (!lm_) {
        return std::nullopt;
    }
    return lm_->Get();
}


log_level LogSystem::GetLevel() noexcept
{
    if (!lm_) {
        return log_level::unknown;
    }
    return lm_->GetLevel();
}


} // namespace logrr
} // namespace uni