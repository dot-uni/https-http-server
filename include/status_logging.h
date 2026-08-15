#ifndef STATUS_LOGGING_INCLUDED
#define STATUS_LOGGING_INCLUDED

#include "logging.h"
#include "status.h"
#include "ret_status.h"

namespace logrr {

class StatusLogger : public Logger 
{
public:
    StatusLogger() = default;
    virtual ~StatusLogger() = default;
public:
    bool log(
        http::retCode code, 
        const std::string& file, 
        int line, const std::string& func, 
        logrr::log_status log_status=logrr::log_status::info
    ) noexcept;
    bool log(
        http::retCode code,
        const std::string& file, 
        int line, 
        const std::string& func, 
        std::vector<LogField>&& add_dtls, 
        logrr::log_status log_status=logrr::log_status::info
    ) noexcept;

    bool log(
        http::status code, 
        const std::string& file, 
        int line, const std::string& func, 
        logrr::log_status log_status=logrr::log_status::info
    ) noexcept;
    bool log(
        http::status code, 
        const std::string& file, 
        int line, 
        const std::string& func, 
        std::vector<LogField>&& add_dtls, 
        logrr::log_status log_status=logrr::log_status::info
    ) noexcept;
};

} // namespace logrr 

#endif