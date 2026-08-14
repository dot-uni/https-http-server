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


Response Router::route(Request&& req) const noexcept 
{
    if (slogger_) slogger_->lCalled(__func__);

    auto handler = rtree_.get(req.method, req.path);
    if (!handler) {
        return makeResp(retCode::NotFound, req.id);
    }
    Response resp = (*handler)(std::forward<Request>(req));
    if (slogger_) slogger_->lExeced(__func__);
    return resp;
}

} // namespace http