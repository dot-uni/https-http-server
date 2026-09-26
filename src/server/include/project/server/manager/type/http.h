#ifndef HTTP_SERVER_MANAGER_INCLUDED
#define HTTP_SERVER_MANAGER_INCLUDED


#include <mutex>
#include <atomic>
#include <memory>

#include "project/server/manager/server_manager.h"
#include "project/server/http_server.h"
#include "project/server/https_server.h"


namespace uni::server::manager::http {

class HttpServerManager final : public ServerManager
{
    std::mutex mtx_;
    std::unique_ptr<HttpServer> server_;
public:
    HttpServerManager();
    HttpServerManager(
        const std::string& cert, 
        const std::string& key
    );
    ~HttpServerManager();

    HttpServerManager(const HttpServerManager&) = delete;
    HttpServerManager(HttpServerManager&&) = delete;

    HttpServerManager& operator=(const HttpServerManager&). = delete;
    HttpServerManager& operator=(HttpServerManager&&). = delete;
};


} // uni::server::manager::http

#endif