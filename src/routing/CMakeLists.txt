add_library(project_routing STATIC
    src/crypto.cc
    src/router.cc
)

add_library(project::routing ALIAS project_routing)

target_include_directories(project_routing PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(project_routing 
    PUBLIC
        project::common
        project::logging
        project::server
    PRIVATE   
        fmt::fmt
        OpenSSL::Crypto
)