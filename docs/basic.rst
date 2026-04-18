The Basics
==========

Overview
--------

DataLayerGenerator is a **header-only C++20 framework** paired with a **Python/Jinja2 code generator**.
You describe your application's data model in JSON or YAML files and run the generator once (or as a CMake
custom target) to produce a single ``datalayer.h`` header.  The generated header uses only types from the
``src/include/`` framework; the framework itself has no runtime dependencies beyond the C++20 standard
library (``fmt`` and ``pybind11`` are optional).

Key properties:

- Every datapoint is a ``constinit`` global — zero heap allocation.
- Access control (``READ_ONLY``, ``WRITE_ONLY``, ``READ_WRITE``) is enforced at compile time via C++20
  Concepts.
- Range checking (``Minimum`` / ``Maximum``) is evaluated at the call site and returns a
  :cpp:enum:`DataLayer::Detail::RangeCheck` value — no exceptions.
- Versioning metadata is embedded in groups and individual datapoints to support EEPROM upgrade paths.
- Optional binary file persistence via :cpp:struct:`Serialization` / :cpp:struct:`Deserialization`
  (enabled with ``-DENABLE_FILE_PERSISTENCE=ON``).
- Optional ``fmt`` formatters for all generated structs and enums (enabled with ``-DENABLE_FMT=ON``).
- Optional Python bindings via ``pybind11`` (enabled with ``-DENABLE_PYBIND11=ON``).
- Cross-compilation support for ARM Cortex-M4 (GCC ARM toolchain).

Architecture
------------

.. code-block:: text

   Model files (JSON / YAML)
   groups.json  datapoints.json  structs.json  enums.json  types.json
          │
          ▼
   generator.py   ←  schema.json  +  Jinja2 templates
          │
     ┌────┴─────────────────────┐
     ▼                          ▼
   generated/include/        generated/datalayer_example/
     datalayer.h               pythonBinding.cpp
     formatter.h
          │
          ▼
   C++ application (embeds constinit datapoint objects)
     #include <include/datalayer.h>
     #include <include/formatter.h>

The ``src/include/`` headers provide the type machinery:

.. code-block:: text

   helper.h
     ├── Version, FixedString, PersistenceType
     └── Helper::READ_ONLY / WRITE_ONLY / READ_WRITE tag types
         ├── detail.h
         │     └── BaseType<T>, RangeCheck, checkValue()
         ├── groupInfo.h
         │     └── DataLayer::GroupInfo<…>
         ├── datapoint.h
         │     └── DataLayer::DataPoint<T, Group, id, Access, …>
         ├── dataPointGroupMapping.h
         │     └── DataLayer::GroupDataPointMapping<GroupInfo, …Datapoints>
         ├── dispatcher.h
         │     └── DataLayer::Dispatcher<…GroupInfos>
         └── serialization.h  (guarded by USE_FILE_PERSISTENCE)
               └── Serialization<Data>, Deserialization<Data>

C++ Framework API
-----------------

GroupInfo
~~~~~~~~~

.. doxygenstruct:: DataLayer::GroupInfo
   :members:

DataPoint
~~~~~~~~~

.. doxygenstruct:: DataLayer::DataPoint
   :members:

GroupDataPointMapping
~~~~~~~~~~~~~~~~~~~~~

.. doxygenstruct:: DataLayer::GroupDataPointMapping
   :members:

Dispatcher
~~~~~~~~~~

.. doxygenstruct:: DataLayer::Dispatcher
   :members:
