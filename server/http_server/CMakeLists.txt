add_library(http_server STATIC
    src/uuid.cc
    src/crypto.cc
    src/http_codec.cc
    src/http_message.cc
    src/http_connection.cc
    src/http_server.cc
)

target_include_directories(http_server PUBLIC
    include
)

target_link_libraries(http_server PUBLIC
    fmt::fmt
    logging
    nlohmann_json::nlohmann_json
    OpenSSL::Crypto
)