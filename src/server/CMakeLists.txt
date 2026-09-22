add_library(project_server
    src/http_codec.cc
    src/http_connection.cc
    src/http_server.cc
    src/https_connection.cc
    src/https_server.cc
)

add_library(project::server ALIAS project_server)

target_include_directories(project_server PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(project_server 
    PUBLIC
        project::logging
        project::routing
        project::common
        OpenSSL::Crypto
        OpenSSL::SSL
    PRIVATE
        fmt::fmt
        nlohmann_json::nlohmann_json
)