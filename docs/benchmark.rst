Benchmarking
============

The optional ``datapoints_benchmark`` executable uses Google Benchmark to measure generated
datapoint access. It is a host-only target and does not participate in normal builds, unit tests,
or cross-compiled firmware builds.

Configure and run an optimized benchmark-only build:

.. code-block:: powershell

   cmake -S . -B build/benchmark -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=OFF -DENABLE_TESTING=OFF -DENABLE_BENCHMARKS=ON
   cmake --build build/benchmark --config Release --target datapoints_benchmark
   .\build\benchmark\test\benchmark\Release\datapoints_benchmark.exe

Google Benchmark is fetched only when ``ENABLE_BENCHMARKS=ON``. The benchmark reuses the
multi-group model from the datapoint Catch2 tests.

Cases
-----

.. list-table::
   :header-rows: 1
   :widths: 35 65

   * - Benchmark output
     - Operation
   * - ``directRead``
     - Reads ``test()`` directly. This measures statically named, type-safe datapoint access and
       excludes runtime ID lookup.
   * - ``dispatchedRead/16388``
     - Reads ``test`` by ID. It is the first datapoint in the first group and represents an early
       successful dispatch.
   * - ``dispatchedRead/16390``
     - Reads ``test`` through its ``renamedFrom`` legacy ID.
   * - ``dispatchedRead/20484``
     - Reads ``testSecond`` by ID, which requires progressing to the second group.
   * - ``dispatchedRead/32770``
     - Reads ``Dp2`` by ID, a readable datapoint near the end of the fixture, representing a late
       successful dispatch.
   * - ``dispatchedRead/4294967295``
     - Looks up an ID absent from the model, exercising an unsuccessful full lookup.

Measurement Details
-------------------

``dispatchedRead`` obtains its ID from a volatile variable, then passes it to
``Dispatcher.getDatapoint``. Google Benchmark prevents the boolean result and destination value
from being optimized away and clobbers memory after each iteration. The benchmark therefore
measures an actual runtime ID path rather than a direct call selected from a compile-time constant.

Direct datapoint access is constant-time. Runtime dispatch uses a generated two-level FKS perfect
hash table containing every current and ``renamedFrom`` absolute ID. It makes two deterministic hash
probes, validates the ID in the resolved slot, and dispatches directly to the typed datapoint access
operation. Successful and missing runtime lookups are therefore $O(1)$ and do not depend on group or
datapoint declaration order.

The generated table is ``constexpr`` and requires no heap allocation or runtime initialization. Its
secondary tables use squared bucket sizes, which gives linear expected ROM consumption while retaining
constant lookup time. Models must not assign the same absolute ID to multiple current or legacy
datapoints; generation rejects that ambiguity. Benchmark a production-sized model on deployment
hardware to evaluate the resulting code size, ROM usage, and timing.
