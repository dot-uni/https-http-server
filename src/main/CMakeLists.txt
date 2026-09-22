add_executable(project_main
    server.cc
)

add_executable(project::main ALIAS project_main)

target_link_libraries(project_main
    PRIVATE
        project::server
        project::routing
        project::logging
)