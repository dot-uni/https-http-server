add_library(http_server STATIC
    src/uuid.cc
    src/http_codec.cc
    src/http_message.cc
    src/http_connection.cc
    src/http_server.cc
    route/src/crypto.cc
    route/src/router.cc
)

target_include_directories(http_server PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/route/include
)

target_link_libraries(http_server PUBLIC
    logging
    fmt::fmt
    nlohmann_json::nlohmann_json
    OpenSSL::Crypto
)