#include <adios2.h>
#include <benchmark/benchmark.h>

static void MetaDataComplexity(benchmark::State& state) {
  std::vector<double> x(state.range(0));
  for (auto _ : state)
  {
    auto y = x;
  }
  state.SetComplexityN(state.range(0));
}
BENCHMARK(MetaDataComplexity)->RangeMultiplier(2)->Range(1 << 3, 1 << 15)->Complexity();

BENCHMARK_MAIN();