#ifndef STATUS_LOGGING_INCLUDED
#define STATUS_LOGGING_INCLUDED

#include "logging.h"
#include "status.h"
#include "ret_status.h"


#define LOG_USING_RETCODE(code, slogger, ...)                                                       \
    do {                                                                                            \
        static_assert(                                                                              \
            logrr::is_slogger_v<std::remove_cvref_t<decltype(*slogger)>>,                           \
            "LOG_USING_RETCODE: 'slogger' must point to logrr::StatusLogger"                        \
        );                                                                                          \
        static_assert(                                                                              \
            std::is_enum_v<std::remove_cvref_t<decltype(code)>>,                                    \
            "The variable 'code' is not an enum type"                                               \
        );                                                                                          \
        auto&& logrr_slogger_ = (slogger);                                                          \
        if (logrr_slogger_ != nullptr) {                                                            \
            logrr_slogger_->log(code, __FILE_NAME__, __LINE__, __func__ __VA_OPT__(,) __VA_ARGS__); \
        }                                                                                           \
    } while (false)


#define LOG_USING_HTTP_STS(status, slogger, ...)                                                        \
    do {                                                                                                \
        static_assert(                                                                                  \
            logrr::is_slogger_v<std::remove_cvref_t<decltype(*slogger)>>,                               \
            "LOG_USING_HTTP_STS: 'slogger' must point to logrr::StatusLogger"                           \
        );                                                                                              \
        static_assert(                                                                                  \
            std::is_enum_v<std::remove_cvref_t<decltype(status)>>,                                      \
            "The variable 'status' is not an enum type"                                                 \
        );                                                                                              \
        auto&& logrr_slogger_ = (slogger);                                                              \
        if (logrr_slogger_ != nullptr) {                                                                \
            logrr_slogger_->log(status, __FILE_NAME__, __LINE__, __func__ __VA_OPT__(,) __VA_ARGS__);   \
        }                                                                                               \
    } while (false)



namespace logrr {

class StatusLogger : public Logger
{
public:
    StatusLogger() = default;
    virtual ~StatusLogger() = default;
public:
    void log(
        http::retCode code, 
        std::string_view file, 
        int line, 
        std::string_view func
    ) noexcept;
    void log(
        http::retCode code,
        std::string_view file, 
        int line, 
        std::string_view func, 
        std::vector<LogField>&& add_dtls
    ) noexcept;

    void log(
        http::status code, 
        std::string_view file, 
        int line, 
        std::string_view func
    ) noexcept;
    void log(
        http::status code, 
        std::string_view file, 
        int line, 
        std::string_view func, 
        std::vector<LogField>&& add_dtls
    ) noexcept;
};

} // namespace logrr 

#endif