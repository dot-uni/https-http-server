cmake_minimum_required(VERSION 3.20)
project(server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(fmt CONFIG REQUIRED)
find_package(nlohmann_json 3.12.0 REQUIRED)
find_package(OpenSSL REQUIRED)

set(SOURCES
    server.cc
    uuid.cc
    status.cc
    log_status.cc
    logging.cc
    status_logging.cc
    router.cc
    routing_tree.cc
    http_message.cc
    http_server.cc
    https_server.cc
    http_connection.cc
    https_connection.cc
    http_codec.cc
)
list(TRANSFORM SOURCES PREPEND impl/)

add_executable(server ${SOURCES})
target_include_directories(server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(server PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(server PRIVATE fmt::fmt)
target_link_libraries(server PRIVATE OpenSSL::SSL OpenSSL::Crypto)