#include "project/server/http/https_server.h"


using namespace uni;

int main(int argc, char** argv) {
    
    if (argc < 4) {
        std::cerr << "Verify that the SSL certificate, private key, and logging configuration are specified\n";
        std::cerr << "\t" << argv[0] << " [cert] [key] [path_to_config]\n";
        return 1;
    }

    logging::manager::LogSystem::Start(argv[3]);

    routing::RouterManager::Init();

    routing::RouterManager::Get("/uni", [](server::http::Request&& req){
        return server::http::makeResp(common::status::retCode::Success, {
            {"Message", "The request was received and successfully processed..."}
        });
    });

    server::http::HttpsServer server(argv[1], argv[2]);
    server.listen();
    return 0;
}   