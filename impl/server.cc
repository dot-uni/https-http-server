#include "https_server.h"


int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Certificate and private key not specified.\nExample:  ./server cert.pem key.pem\n";
        return 1;
    }
    https::HttpsServer server(argv[1], argv[2]);
    server.addSink(std::make_shared<logrr::FileSink>());
    server.listen();
    return 0;
}   