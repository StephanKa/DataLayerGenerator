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

Direct datapoint access is constant-time. In source, the dispatcher visits group mappings and their
datapoints in declaration order, stopping at a matching ID; an unknown ID visits all of them. This
makes the source-level worst case linear in the number of groups and datapoints. However, an
optimizing compiler may simplify comparisons for a small fixed model, so use a model with
production-like group and datapoint counts and run the benchmark on the deployment hardware before
making timing or scaling decisions.
