add_executable(project_bench 
    bench_test.cc
)

add_executable(project::bench ALIAS project_bench)

target_link_libraries(project_bench 
    PUBLIC
        spdlog::spdlog
        benchmark::benchmark_main
        project::logging
)