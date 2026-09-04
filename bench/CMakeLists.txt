add_executable(runnable_benchmarks 
    bench_test.cc
)

target_link_libraries(runnable_benchmarks PRIVATE
    benchmark::benchmark_main
    https_server
    http_server
    logging
)