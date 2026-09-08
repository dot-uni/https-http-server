#include "https_server.h"


int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "Verify that the SSL certificate, private key, and logging configuration are specified\n";
        std::cerr << "\t" << argv[0] << " [cert] [key] [path_to_config]\n";
        return 1;
    }

    http::Router<> router;
    router.get("/", [](http::Request&& req){
        return http::makeResp(http::retCode::Success, {
            {"Message", "The request was received and successfully processed..."}
        });
    });

    logrr::LogManager::Init(argv[3]);
    https::HttpsServer server(argv[1], argv[2], router);
    server.listen();
    return 0;
}   