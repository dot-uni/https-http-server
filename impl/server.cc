#include "https_server.h"

/*
    Example
*/


int main() {
    https::HttpsServer server;
    server.addSink(std::make_shared<logrr::FileSink>());
    server.listen();
}   