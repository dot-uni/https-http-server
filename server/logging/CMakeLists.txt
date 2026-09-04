add_library(logging STATIC
    src/log_status.cc
    src/logging.cc
)

target_include_directories(logging PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(logging PUBLIC
    fmt::fmt
)