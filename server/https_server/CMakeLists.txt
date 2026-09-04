add_library(https_server STATIC
    src/https_connection.cc
    src/https_server.cc
)

target_include_directories(https_server PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(https_server PUBLIC
    http_server
    logging
    OpenSSL::SSL
    OpenSSL::Crypto
)