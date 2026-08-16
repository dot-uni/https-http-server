#include "status_logging.h"

namespace logrr {


/**
 * log with RetCode
 */

bool StatusLogger::log(
    http::retCode code, 
    const std::string& file, 
    int line, 
    const std::string& func, 
    logrr::log_status log_status
) noexcept
{
    http::status s = http::toHttpStatus(code);
    return Logger::log(LogRecord{
        .status = log_status,
        .file = file,
        .line = line,
        .func = func,
        .details = {
            logrr::field("retCode", code),
            logrr::field("retMesg", http::retMesg(code)),
            logrr::field("status", s),
            logrr::field("obsolete_reason", http::obsolete_reason(s))
        }
    });
}

 
bool StatusLogger::log(
    http::retCode code, 
    const std::string& file, 
    int line, 
    const std::string& func, 
    std::vector<LogField>&& add_dtls, 
    logrr::log_status log_status
) noexcept
{
    http::status s = http::toHttpStatus(code);
    std::vector<LogField> dtls = {
        logrr::field("retCode", code),
        logrr::field("retMesg", http::retMesg(code)),
        logrr::field("status", s),
        logrr::field("obsolete_reason", http::obsolete_reason(s))
    };
    dtls.insert(dtls.end(), add_dtls.begin(), add_dtls.end());
    return Logger::log(LogRecord{
        .status = log_status,
        .file = file,
        .line = line,
        .func = func,
        .details = std::move(dtls)
    });
}


/**
 * log with HTTP status
 */

bool StatusLogger::log(
    http::status code, 
    const std::string& file, 
    int line, 
    const std::string& func, 
    logrr::log_status log_status
) noexcept 
{
    return Logger::log(LogRecord{
        .status = log_status,
        .file = file,
        .line = line,
        .func = func,
        .details = {
            logrr::field("status", code),
            logrr::field("obsolete_reason", http::obsolete_reason(code))
        }
    });
}


bool StatusLogger::log(
    http::status code, 
    const std::string& file, 
    int line, 
    const std::string& func, 
    std::vector<LogField>&& add_dtls, 
    logrr::log_status log_status
) noexcept
{
    std::vector<LogField> dtls = {
        logrr::field("status", code),
        logrr::field("obsolete_reason", http::obsolete_reason(code))
    };

    dtls.insert(dtls.end(), add_dtls.begin(), add_dtls.end());
    return Logger::log(LogRecord{
        .status = log_status,
        .file = file,
        .line = line,
        .func = func,
        .details = std::move(dtls)
    });
}

} // namespace logrr