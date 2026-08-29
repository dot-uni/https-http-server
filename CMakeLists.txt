cmake_minimum_required(VERSION 3.20)
project(server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(fmt CONFIG REQUIRED)
find_package(nlohmann_json 3.12.0 REQUIRED)
find_package(OpenSSL REQUIRED)

add_subdirectory(server)

add_executable(server_app server/http_server/impl/server.cc)
set_target_properties(server_app PROPERTIES OUTPUT_NAME "server")
target_link_libraries(server_app PRIVATE https_server http_server logging)