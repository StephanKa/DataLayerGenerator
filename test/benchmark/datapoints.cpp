#include <benchmark/benchmark.h>
#include <datalayer.h>

#include <cstdint>
#include <limits>

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
        const bool expectedFound = dataPointId != std::numeric_limits<uint32_t>::max();

        for (auto _ : state)
        {
            const bool found = Dispatcher.getDatapoint(dataPointId, value);
            if (found != expectedFound)
            {
                state.SkipWithError("dispatcher lookup returned an unexpected result");
                break;
            }
            benchmark::DoNotOptimize(found);
            benchmark::DoNotOptimize(value);
            benchmark::ClobberMemory();
        }
    }

    BENCHMARK(directRead);
    BENCHMARK(dispatchedRead)->Arg(test.getId())->Arg(0x4006U)->Arg(testSecond.getId())->Arg(Dp2.getId())->Arg(0xffff'ffffU);
}// namespace
