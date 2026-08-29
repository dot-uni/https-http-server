add_library(http_server STATIC
    impl/uuid.cc
    impl/status.cc
    impl/crypto.cc
    impl/routing_tree.cc
    impl/http_codec.cc
    impl/http_message.cc
    impl/http_connection.cc
    impl/http_server.cc
)

target_include_directories(http_server PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(http_server PUBLIC
    logging
    nlohmann_json::nlohmann_json
    OpenSSL::Crypto
)