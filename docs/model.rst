Model File Reference
====================

The data model is described in JSON (or YAML) files placed in a single directory.  All files in the
directory are merged before validation, so you can split the model across as many files as you like.
Only the top-level keys ``Groups``, ``Datapoints``, ``Structs``, ``Enums``, and ``Types`` are
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

When ``"min"`` and ``"max"`` are present, :cpp:func:`DataLayer::Detail::checkValue` enforces the range
on every ``set()`` call and returns :cpp:enum:`DataLayer::Detail::RangeCheck`:

- ``underflow`` if the value is below ``min``
- ``overflow`` if the value is above ``max``
- ``ok`` if within range

YAML Support
------------

All model files can be written in YAML instead of JSON.  The generator accepts any mix of ``.json``
and ``.yaml`` / ``.yml`` files in the model directory.  Use ``--convert`` to convert an existing JSON
model to YAML.
</content>
</invoke>
