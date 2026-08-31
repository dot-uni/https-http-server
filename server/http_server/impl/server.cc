#include "https_server.h"


int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Certificate and private key not specified.\nExample:  ./server cert.pem key.pem\n";
        return 1;
    }

    http::Router<> router;
    router.get("/", [](http::Request&& req){
        return http::makeResp(http::retCode::Success, {
            {"Message", "The request was received and successfully processed..."}
        });
    });

    auto logger = std::make_shared<logrr::Logger>();

    logger->add_sink<logrr::ConsoleSink<>>();
    logger->add_sink<logrr::FileSink<>>();

    https::HttpsServer server(argv[1], argv[2], router, logger);
    server.listen();
    return 0;
}   