Model File Reference
====================

The data model is described in JSON (or YAML) files placed in a single directory.  All files in the
directory are merged before validation, so you can split the model across as many files as you like.
Only the top-level keys ``Groups``, ``Datapoints``, ``Processors``, ``Structs``, ``Enums``, and ``Types`` are
recognised.

groups.json
-----------

Defines one or more **groups**.  Each datapoint belongs to exactly one group.  A group carries a
``baseId`` — the base address for the ID space — and a ``version`` used by the serialization layer.

.. code-block:: json

   {
     "Groups": [
       {
         "name":        "DefaultGroup",
         "persistence": "None",
         "baseId":      "0x4000",
         "version":     "1.0.1",
         "description": "Default group with no persistence."
       },
       {
         "name":        "CyclicGroup",
         "persistence": "Cyclic",
         "baseId":      "0x5000",
         "version":     "2.0.0",
         "description": "Group persisted cyclically to external memory."
       }
     ]
   }

.. list-table::
   :header-rows: 1
   :widths: 20 15 65

   * - Field
     - Required
     - Description
   * - ``name``
     - Yes
     - Unique group name.  Used as the C++ variable name in the generated header.
   * - ``baseId``
     - Yes
     - Base address (hex string or integer).  All datapoint IDs in this group are offset from here.
   * - ``version``
     - Yes
     - Semantic version string ``"major.minor.build"`` used for deserialization version checks.
   * - ``persistence``
     - No
     - One of ``"None"`` (default), ``"Cyclic"``, ``"OnWrite"``.
   * - ``description``
     - No
     - Human-readable description (informational only).
  * - ``maxPayloadBytes``
    - No
    - Non-negative portable payload-RAM budget for this group. The generated header asserts the
     sum of its datapoint value sizes does not exceed this limit.

Memory Budgets
--------------

Use ``maxPayloadBytes`` to keep a group within its model-level payload budget:

.. code-block:: json

  {
    "name": "CyclicGroup",
    "persistence": "Cyclic",
    "baseId": "0x5000",
    "version": "2.0.0",
    "maxPayloadBytes": 256
  }

The assertion counts each stored value, including all array elements, but excludes DataLayer
bookkeeping. Configure the board-specific full object budget in the consuming CMake target when
you also want to include the four observer callback/context slots stored by every ``DataPoint``:

.. code-block:: cmake

  target_compile_definitions(my_firmware PRIVATE DATALAYER_MAX_OBJECT_BYTES=1024)

The generated header asserts this total across all generated datapoints. Both checks are
compile-time only and add no runtime memory or execution cost.

datapoints.json
---------------

Defines the individual **datapoints**.

.. code-block:: json

   {
     "Datapoints": [
       {
         "name":      "test",
         "group":     "DefaultGroup",
         "id":        1,
         "type":      "int32_t",
         "default":   4211,
         "access":    "READ_WRITE",
         "namespace": "Testify",
         "version":   "1.0.1",
         "description": "Test int32_t datapoint."
       },
       {
         "name":    "test4",
         "group":   "CyclicGroup",
         "id":      4,
         "type":    "Temperature",
         "default": { "value": 123.0, "raw": 5555 },
         "access":  "READ_WRITE",
         "version": "1.0.0"
       }
     ]
   }

.. list-table::
   :header-rows: 1
   :widths: 20 15 65

   * - Field
     - Required
     - Description
   * - ``name``
     - Yes
     - Unique name within the group.
   * - ``group``
     - Yes
     - Name of the group this datapoint belongs to.
   * - ``id``
     - Yes
     - Offset from the group's ``baseId``.  Must be unique within the group.
   * - ``type``
     - Yes
     - One of the C++ base types (``int32_t``, ``float``, ``bool``, …), a defined struct name,
       enum name, or alias type name.
   * - ``default``
     - No
     - Default value.  For struct types use a JSON object with one key per struct field.
       For array types use a JSON array.
   * - ``access``
     - Yes
     - ``"READ_ONLY"``, ``"WRITE_ONLY"``, or ``"READ_WRITE"``.
   * - ``namespace``
     - No
     - Optional C++ namespace for the generated global variable.
   * - ``version``
     - No
     - Semantic version ``"major.minor.build"`` for the datapoint (default ``"0.0.0"``).
   * - ``description``
     - No
     - Human-readable description.
   * - ``renamedFrom``
     - No
     - Array of prior group-relative numeric IDs. Generated dispatch accepts these IDs for the datapoint.
   * - ``migration``
     - No
     - Name of a byte-level C++ migration callback used when an allowed persistence upgrade changes payload size.

Model Evolution
---------------

Use ``renamedFrom`` when a datapoint keeps its type but moves to a new group-relative ID:

.. code-block:: json

   {
     "name": "temperature", "group": "CyclicGroup", "id": 12,
     "renamedFrom": [4], "type": "int32_t", "access": "READ_WRITE", "version": "2.0.0"
   }

For a persisted type-size change, set ``allowUpgrade`` to ``true`` and declare ``migration``. The
generator declares the callback in ``DataLayer::Migration``; the application must define it:

.. code-block:: cpp

   bool DataLayer::Migration::migrateTemperature(
       const Version& sourceVersion,
       std::span<const std::byte> source,
       std::span<std::byte> destination) noexcept
   {
       // Decode source and write the destination representation.
       return true;
   }

The callback must return ``false`` when it cannot migrate the supplied bytes.

processors.json
---------------

Defines optional input-process-output declarations. Processors are generated only when consumers
enable ``-DENABLE_IPO_PROCESSORS=ON``. The processor's application function remains application-owned:
the generator declares it in ``DataLayer::Ipo::Application`` and does not impose a scheduler, task,
or allocation strategy.

.. code-block:: json

   {
     "Processors": [
       {
         "name": "TemperatureControl",
         "function": "updateTemperatureControl",
         "inputs": ["Plant::setpoint", "Plant::measuredTemperature"],
         "outputs": ["Plant::heaterCommand"],
         "execution": "on_change"
       }
     ]
   }

``name`` and ``function`` must be unique C++ identifiers. ``inputs`` and ``outputs`` use generated
datapoint names, including ``namespace::name`` when applicable. Inputs must not be ``WRITE_ONLY``;
outputs must not be ``READ_ONLY``; a datapoint cannot be both for the same processor. The generator
also rejects duplicate references and cycles in the graph formed when one processor output feeds
another processor input.

``execution`` defaults to ``"manual"``. A manual processor exposes ``process()`` only; call it at a
known point in the application's control loop. An ``"on_change"`` processor also exposes
``initialize()`` and ``processIfChanged()``. Call ``initialize()`` once during startup and check its
return value: it registers one existing allocation-free observer per input and can fail if an input's
observer capacity is already exhausted. Input writes only mark the processor dirty;
``processIfChanged()`` invokes the application function once and returns ``true`` when work ran.

.. code-block:: cpp

   void DataLayer::Ipo::Application::updateTemperatureControl() noexcept
   {
       const auto setpoint = Plant::setpoint();
       const auto measured = Plant::measuredTemperature();
       std::ignore = Plant::heaterCommand.set(calculateCommand(setpoint, measured));
   }

   bool initializeControl()
   {
       return DataLayer::Ipo::TemperatureControl::initialize();
   }

   void controlLoop()
   {
       DataLayer::Ipo::TemperatureControl::processIfChanged();
   }

The dirty flag is deliberately not synchronized. The application owns task, interrupt, and lock
policy: call ``processIfChanged()`` from one scheduling context, protect concurrent input writes as
required by the target, and avoid declaring outputs that indirectly cause input writes. Processors
do not run inline in a datapoint callback, so transactional input batches can complete before the
next scheduled processing pass.

structs.json
------------

Defines custom **struct types** that can be used as datapoint types.  Structs can contain scalar
fields or other struct fields (nested structs).

.. code-block:: json

   {
     "Structs": [
       {
         "name": "Temperature",
         "parameter": [
           { "value": "float" },
           { "raw":   "uint32_t" }
         ]
       },
       {
         "name": "Environment",
         "parameter": [
           { "internal": "Temperature" },
           { "external": "Temperature" }
         ]
       }
     ]
   }

enums.json
----------

Defines custom **enum types**.  Two modes:

**Auto-ID** (``"autoId": true``) — values assigned starting at 0:

.. code-block:: json

   {
     "Enums": [
       {
         "name":   "Status",
         "type":   "uint32_t",
         "values": ["Booting", "Starting", "Finished"],
         "autoId": true
       }
     ]
   }

**Explicit values** (``"autoId": false``) — each entry is a ``{ "EnumeratorName": intValue }`` object:

.. code-block:: json

   {
     "Enums": [
       {
         "name":   "ErrorStates",
         "type":   "uint32_t",
         "autoId": false,
         "values": [
           { "None": 0 }, { "Info": 1 }, { "Warn": 4 },
           { "Error": 5 }, { "Critical": 9 }, { "Fatal": 10 }
         ]
       }
     ]
   }

  For every generated enum ``Example``, the generated header also declares an ``inline constexpr``
  ``std::array`` named ``ExampleRange``. It contains the declared enumerators in generated order and
  can be iterated directly. This includes explicitly assigned, non-consecutive values:

  .. code-block:: cpp

     for (const auto status : StatusRange)
     {
       // Use status.
     }

types.json
----------

Defines **alias types** — thin wrappers around a base type with optional compile-time range bounds.

.. code-block:: json

   {
     "Types": [
       {
         "name": "AliasTypeUint32t",
         "type": "uint32_t",
         "min":  100,
         "max":  200
       },
       {
         "name": "AliasTypeFloat",
         "type": "float"
       }
     ]
   }

Structured Alias Validation
---------------------------

Alias types can add validation evaluated by the same ``DataLayer::Detail::checkValue`` path used by
direct datapoint writes, group mappings, dispatchers, and transactional updates. A rejected write
does not modify the stored value and returns the relevant ``RangeCheck`` result.

Numeric integer aliases support ``min``, ``max``, a positive ``step`` anchored at ``min``, and an
``allowedValues`` list. ``step`` is intentionally restricted to integer aliases because exact
floating-point step arithmetic is not reliable in C++23 constant evaluation.

.. code-block:: json

   {
     "name": "FanPercent",
     "type": "uint8_t",
     "min": 0,
     "max": 100,
     "step": 5,
     "allowedValues": [0, 25, 50, 75, 100]
   }

``std::string_view`` aliases support ``minLength``, ``maxLength``, ``allowedValues``, and a
constexpr glob ``pattern``. ``?`` matches one character and ``*`` matches zero or more characters;
this deliberately avoids a runtime regular-expression dependency.

.. code-block:: json

   {
     "name": "DeviceLabel",
     "type": "std::string_view",
     "minLength": 3,
     "maxLength": 12,
     "pattern": "DEV-????"
   }

The returned validation errors are ``underflow``, ``overflow``, ``stepMismatch``, ``notAllowed``,
``lengthUnderflow``, ``lengthOverflow``, and ``patternMismatch``. All rule metadata and allowed
value lists are generated as ``constexpr`` objects; glob matching is constexpr when the input is.

When ``"min"`` and ``"max"`` are present, :cpp:func:`DataLayer::Detail::checkValue` enforces the range
on every ``set()`` call and returns :cpp:enum:`DataLayer::Detail::RangeCheck`:

- ``underflow`` if the value is below ``min``
- ``overflow`` if the value is above ``max``
- ``ok`` if within range

VS Code Schema Validation
-------------------------

Associate the bundled ``src/generator/schema.json`` with model files in the workspace settings to
get completion and validation diagnostics while editing. Create or update ``.vscode/settings.json``
in the repository root:

.. code-block:: json

   {
     "json.schemas": [
       {
         "fileMatch": ["/src/model/*.json"],
         "url": "./src/generator/schema.json"
       }
     ],
     "yaml.schemas": {
       "./src/generator/schema.json": ["src/model/*.yaml", "src/model/*.yml"]
     }
   }

JSON Schema support is built into VS Code. For YAML model files, install the `YAML extension by
Red Hat <https://marketplace.visualstudio.com/items?itemName=redhat.vscode-yaml>`_. Adjust the
``fileMatch`` patterns when the model directory is elsewhere. Do not add a ``$schema`` property to
model files: the generator validates top-level keys strictly, and the editor association keeps that
metadata out of the model.

YAML Support
------------

All model files can be written in YAML instead of JSON.  The generator accepts any mix of ``.json``
and ``.yaml`` / ``.yml`` files in the model directory.  Use ``--convert`` to convert an existing JSON
model to YAML.
</content>
</invoke>
