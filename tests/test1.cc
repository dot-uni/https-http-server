#include <benchmark/benchmark.h>
#include <vector>
#include <algorithm>


static void BM_Sort(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();

        state.ResumeTiming();

        std::sort(v.begin(), v.end());
    }
}
BENCHMARK(BM_Sort)->Arg(1000)->Arg(10000);

BENCHMARK_MAIN();