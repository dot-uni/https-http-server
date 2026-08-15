#include "router.h"

namespace http {

Router::Router(std::shared_ptr<logrr::ILogSink> logsink) 
{
    if (logsink) {
        slogger_ = std::make_shared<logrr::StatusLogger>();
        if (slogger_ && !slogger_->addSink(logsink)) {
            slogger_.reset();
            slogger_ = nullptr;
        }
    }
}

Router::Router(std::shared_ptr<logrr::StatusLogger> slogger) : slogger_(slogger) {}


bool Router::get(const std::string& path, Handler h) noexcept
{
    return rtree_.add(Method::GET, path, h);
}


bool Router::post(const std::string& path, Handler h) noexcept
{
    return rtree_.add(Method::POST, path, h);
}


bool Router::put(const std::string& path, Handler h) noexcept
{
    return rtree_.add(Method::PUT, path, h);
}


bool Router::del(const std::string& path, Handler h) noexcept
{
    return rtree_.add(Method::DELETE, path, h);
}


bool Router::patch(const std::string& path, Handler h) noexcept
{
    return rtree_.add(Method::PATCH, path, h);
}


Response Router::route(Request&& req) const noexcept 
{
    auto handler = rtree_.get(req.method, req.path);
    if (!handler) {
        if (slogger_) slogger_->log(retCode::NotFound, __FILE_NAME__, __LINE__, __func__, {
            logrr::field("method", req.method),
            logrr::field("path", req.path)
        }, logrr::log_status::warning);
        return makeResp(retCode::NotFound, req.id);
    }
    Response resp = (*handler)(std::move(req));
    if (slogger_) slogger_->log(resp.status, __FILE_NAME__, __LINE__, __func__, {
        logrr::field("method", req.method),
        logrr::field("path", req.path)
    }, to_log_status(resp.status)); 
    return resp;
}

} // namespace http