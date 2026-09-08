add_library(logging STATIC
    src/status.cc
    src/log_level.cc
    src/logging.cc
)

target_include_directories(logging PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(logging PUBLIC
    fmt::fmt
    yaml-cpp::yaml-cpp
    nlohmann_json::nlohmann_json
)