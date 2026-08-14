#include "https_server.h"


int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Certificate and private key not specified.\nExample:  ./server cert.pem key.pem\n";
        return 1;
    }

    auto logsink = std::make_shared<logrr::ConsoleSink>();

    http::Router router(logsink);
    router.get("/", [](http::Request&& req){
        return http::makeResp(http::retCode::Success, {
            {"Message", "The request was received and successfully processed..."}
        });
    });

    https::HttpsServer server(argv[1], argv[2], router, logsink);
    server.addSink(std::make_shared<logrr::FileSink>());
    server.listen();
    return 0;
}   