Examples
========

The following examples use the model files in ``src/model/`` that ship with the project.
After running the generator the produced ``datalayer.h`` exposes all datapoints as ``constinit``
global objects.

Reading and Writing a Datapoint
--------------------------------

.. code-block:: cpp

   #include <include/datalayer.h>   // generated header

   // READ_WRITE int32_t datapoint in namespace Testify
   std::ignore = Testify::test.set(42);       // returns RangeCheck::ok
   int32_t val = Testify::test.get();         // val == 42
   int32_t raw = Testify::test();             // operator() always available

Range-Checked Alias Types
--------------------------

The model defines ``AliasTypeUint32t`` with ``"min": 100, "max": 200``.

.. code-block:: cpp

   auto rc = TestAlias.set(AliasTypeUint32t{42});
   // rc == DataLayer::Detail::RangeCheck::underflow  (42 < 100)

   auto rc2 = TestAlias.set(AliasTypeUint32t{150});
   // rc2 == DataLayer::Detail::RangeCheck::ok

Struct Datapoints
-----------------

The model defines a ``Temperature`` struct with ``value: float`` and ``raw: uint32_t``.

.. code-block:: cpp

   Temperature t{ .value = 23.5f, .raw = 2350 };
   std::ignore = test4.set(t);

   const Temperature &stored = test4.get();
   // stored.value == 23.5f

Array Datapoints
----------------

.. code-block:: cpp

   // Read all elements
   const auto &arr = arrayTest.get();   // returns std::array<…, N>&
   for (const auto &elem : arr)
       fmt::print("{}\n", elem);

   // Write a single element
   arrayTest.set(2, 99);   // index 2

Dispatcher — Runtime-ID Access
--------------------------------

The ``Dispatcher`` allows dynamic get/set by integer datapoint ID across all groups:

.. code-block:: cpp

   auto result = Dispatcher.setDatapoint(0x4001, 42);
   // result.success == true  when ID 0x4001 exists and type matches
   // result.check   == DataLayer::Detail::RangeCheck::ok  (or underflow/overflow)

   int32_t out{};
   bool found = Dispatcher.getDatapoint(0x4001, out);

Printing the Data Layer Structure
----------------------------------

When ``ENABLE_FMT`` is set:

.. code-block:: cpp

   // Print all group names and datapoint IDs
   DefaultGroup.printDatapoints();
   CyclicGroup.printDatapoints();

   // Print the entire hierarchy
   Dispatcher.printStructure();

File Persistence
----------------

When ``ENABLE_FILE_PERSISTENCE`` is set:

.. code-block:: cpp

   // Write CyclicGroup to binary file
   auto status = CyclicGroup.serializeGroup("data/cyclic.bin");
   if (!status.result)
       fmt::print("Serialization failed: {}\n", static_cast<int>(status.errorCode));

   // Read back — version check happens automatically
   auto status2 = CyclicGroup.deserializeGroup("data/cyclic.bin");

Python Bindings
---------------

When ``ENABLE_PYBIND11`` is set the generator produces ``pythonBinding.cpp`` and CMake builds a
``datalayer_example`` Python extension. Build and install its wheel from the repository root:

.. code-block:: powershell

   uv venv
   uv pip install --python .venv\Scripts\python.exe build
   Push-Location datalayer_example
   uv build
   uv pip install --python ..\.venv\Scripts\python.exe --force-reinstall .\dist\datalayer_example-0.0.1-*.whl
   Pop-Location

The generated module contains one Python class for each model struct and one static ``get``/``set``
class for each datapoint. The following examples use the model files shipped in ``src/model/``.

Scalar Datapoints
~~~~~~~~~~~~~~~~~

.. code-block:: python

   import datalayer_example as dl

   print(dl.__version__)
   dl.Test.set(42)
   print(dl.Test.get())   # 42

Struct Datapoints
~~~~~~~~~~~~~~~~~

``Temperature`` is generated from the model's struct definition. Its fields are regular mutable
Python attributes. Constructor arguments follow the generated field order, but assigning fields
after construction is often clearer:

.. code-block:: python

   temperature = dl.Temperature(2350, 23.5)
   temperature.raw = 2400
   temperature.value = 24.0

   dl.Test4.set(temperature)
   current = dl.Test4.get()
   print(current.raw, current.value)  # 2400 24.0

Nested Struct Datapoints
~~~~~~~~~~~~~~~~~~~~~~~~

The same pattern works for structs containing other generated structs:

.. code-block:: python

   internal = dl.Temperature(100, 18.5)
   external = dl.Temperature(200, 21.0)
   environment = dl.Environment(internal, external)

   dl.Structinstructtype.set(environment)
   current = dl.Structinstructtype.get()
   print(current.internal.value, current.external.value)  # 18.5 21.0

Fixed-Size Array Datapoints
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Fixed-size C++ ``std::array`` datapoints accept and return Python sequences. The supplied sequence
must contain exactly the model's declared number of elements:

.. code-block:: python

   dl.Arraytest2.set([1, 2, 3, 4, 5, 6, 7, 8, 9, 10])
   print(dl.Arraytest2.get())

For a custom module name, configure CMake with ``-DPYBIND11_MODULE_NAME=my_datalayer``. Import the
extension using that exact name after installing it.
