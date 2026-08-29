#include "status_logging.h"


namespace logrr {

/**
 * log with RetCode
 */

void StatusLogger::log(
    http::retCode code, 
    std::string_view file, 
    int line, 
    std::string_view func
) noexcept
{
    http::status s = http::to_http_status(code);

    Logger::log(http::to_log_status(code), file, line, func, {
        logrr::field("retCode", code),
        logrr::field("retMesg", http::retMesg(code)),
        logrr::field("status", s),
        logrr::field("obsolete_reason", http::obsolete_reason(s))
    });
}


void StatusLogger::log(
    http::retCode code, 
    std::string_view file, 
    int line, 
    std::string_view func, 
    std::vector<LogField>&& add_dtls
) noexcept
{
    http::status s = http::to_http_status(code);
    std::vector<LogField> dtls = {
        logrr::field("retCode", code),
        logrr::field("retMesg", http::retMesg(code)),
        logrr::field("status", s),
        logrr::field("obsolete_reason", http::obsolete_reason(s))
    };
    dtls.insert(dtls.end(), add_dtls.begin(), add_dtls.end());

    Logger::log(http::to_log_status(code), file, line, func, std::move(dtls));
}


/**
 * log with HTTP status
 */

void StatusLogger::log(
    http::status code, 
    std::string_view file, 
    int line, 
    std::string_view func
) noexcept 
{
    Logger::log(http::to_log_status(code), file, line, func, {
        logrr::field("status", code),
        logrr::field("obsolete_reason", http::obsolete_reason(code))
    });
}


void StatusLogger::log(
    http::status code, 
    std::string_view file, 
    int line, 
    std::string_view func, 
    std::vector<LogField>&& add_dtls
) noexcept
{
    std::vector<LogField> dtls = {
        logrr::field("status", code),
        logrr::field("obsolete_reason", http::obsolete_reason(code))
    };

    dtls.insert(dtls.end(), add_dtls.begin(), add_dtls.end());

    Logger::log(http::to_log_status(code), file, line, func, std::move(dtls));
}

} // namespace logrr