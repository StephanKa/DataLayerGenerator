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

When ``ENABLE_PYBIND11`` is set the generator produces ``pythonBinding.cpp`` and a ``conan_pybind11``
shared library.  After installing the generated ``datalayer_example`` package:

.. code-block:: python

   import conan_pybind11 as dl

   dl.test_set(42)
   print(dl.test_get())   # 42
