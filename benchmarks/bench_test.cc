#include <benchmark/benchmark.h>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/spdlog.h>
#include "project/logging/logging.h"


/// There are no benchmarks yet
#if 0
constexpr std::string_view kMessage =
    "[2026-09-03 23:00:10] [INFO] source.cc:(21:5) test message";

std::shared_ptr<logrr::Logger> make_logrr_logger() {
    auto logger = std::make_shared<logrr::Logger>();
    logger->add_sink<logrr::ConsoleSink<>>(); // или эквивалент sink без I/O
    return logger;
}

auto logrr_logger = make_logrr_logger();

auto spd_logger = [] {
    auto sink = std::make_shared<spdlog::sinks::null_sink_mt>();
    auto logger = std::make_shared<spdlog::logger>("bench", sink);
    logger->set_level(spdlog::level::info);
    logger->set_pattern("%v"); // настройте аналогично logrr
    return logger;
}();

static void BM_spdlog(benchmark::State& state) {
    for (auto _ : state) {
        spd_logger->info("{}", kMessage);
        benchmark::DoNotOptimize(spd_logger);
    }
}
BENCHMARK(BM_spdlog)->Iterations(1'000'000);

static void BM_logrr(benchmark::State& state) {
    for (auto _ : state) {
        logrr::log_info(logrr_logger.get());
        benchmark::DoNotOptimize(logrr_logger);
    }
}
BENCHMARK(BM_logrr)->Iterations(1'000'000);
#endif