add_library(project_common STATIC
    src/status/log.cc
    src/status/http.cc
    src/uuid.cc
)

add_library(project::common ALIAS project_common)

target_include_directories(project_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(project_common
    PRIVATE
        fmt::fmt
        nlohmann_json::nlohmann_json
)