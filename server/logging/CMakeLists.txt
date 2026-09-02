add_library(logging STATIC
    impl/log_status.cc
    impl/logging.cc
)

target_include_directories(logging PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(logging PUBLIC
    fmt::fmt
)