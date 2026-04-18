File Persistence (Serialization)
================================

When the project is built with ``-DENABLE_FILE_PERSISTENCE=ON`` (compiles with ``-DUSE_FILE_PERSISTENCE``),
the serialization layer in ``serialization.h`` is available.  It provides binary file I/O for entire
groups with group- and datapoint-level version validation.

Enabling Persistence
--------------------

Set ``"persistence"`` in ``groups.json``:

.. code-block:: json

   {
     "name": "CyclicGroup",
     "persistence": "Cyclic",
     "baseId": "0x5000",
     "version": "2.0.0"
   }

Allowed values: ``"None"`` (default), ``"Cyclic"``, ``"OnWrite"``.

The generated ``GroupDataPointMapping`` exposes two methods when  ``USE_FILE_PERSISTENCE`` is defined:

.. code-block:: cpp

   SerializationStatus CyclicGroup.serializeGroup("data/cyclic.bin");
   SerializationStatus CyclicGroup.deserializeGroup("data/cyclic.bin");

Version-Aware Upgrade
---------------------

Each group carries a ``Version { major, minor, build }`` and each datapoint carries its own version.
During deserialization:

1. The stored group version is compared to the current group version.
2. If they differ and ``AllowUpgrade`` is ``true`` on the group, deserialization continues
   and skips individual datapoints whose stored version does not match the current version.
3. If ``AllowUpgrade`` is ``false`` on the group a version mismatch sets
   ``SerializationError::GroupVersion`` and deserialization stops.

API Reference
-------------

.. doxygenstruct:: SerializationStatus
   :members:

.. doxygenenum:: SerializationError

.. doxygenstruct:: Serialization
   :members:

.. doxygenstruct:: Deserialization
   :members:
