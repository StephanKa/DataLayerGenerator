#include <benchmark/benchmark.h>
#include <datalayer.h>

#include <cstdint>

namespace
{
    void directRead(benchmark::State &state)
    {
        for (auto _ : state)
        {
            benchmark::DoNotOptimize(test());
        }
    }

    void dispatchedRead(benchmark::State &state)
    {
        Temperature value{};
        volatile uint32_t dataPointId = static_cast<uint32_t>(state.range(0));

        for (auto _ : state)
        {
            const bool found = Dispatcher.getDatapoint(dataPointId, value);
            benchmark::DoNotOptimize(found);
            benchmark::DoNotOptimize(value);
            benchmark::ClobberMemory();
        }
    }

    BENCHMARK(directRead);
    BENCHMARK(dispatchedRead)->Arg(test.getId())->Arg(testSecond.getId())->Arg(Dp2.getId())->Arg(0xffff'ffffU);
}// namespace
