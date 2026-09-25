add_library(project_logging STATIC
    src/logging.cc
    src/logging_manager.cc
    src/logging_system.cc
    src/config_watcher.cc
)

if(APPLE)
    target_sources(project_logging PRIVATE src/config_watcher_impl/macos.cc)
endif()

add_library(project::logging ALIAS project_logging)

target_include_directories(project_logging PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(project_logging 
    PUBLIC
        project::common
        fmt::fmt
        yaml-cpp::yaml-cpp
        nlohmann_json::nlohmann_json
)