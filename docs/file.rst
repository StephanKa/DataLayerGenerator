File Persistence (Serialization)
================================

When the project is built with ``-DENABLE_FILE_PERSISTENCE=ON`` (compiles with ``-DUSE_FILE_PERSISTENCE``),
the serialization layer in ``serialization.h`` is available.  It provides binary file I/O for entire
groups with group- and datapoint-level version validation.

Format and Integrity
--------------------

Persistence writes the ``DLG1`` format only. Each file has a magic value, format version, group ID,
group version, and CRC32 checksum. Each datapoint is stored as an ID-tagged, versioned,
length-delimited record. Unknown records can therefore be skipped when a newer model adds fields.
Writes stage data in a ``.tmp`` file before replacing the target file.

Files without the ``DLG1`` header are rejected with ``SerializationError::InvalidFormat``. The reader
also rejects unsupported format versions, invalid record bounds, files larger than 16 MiB, checksum
failures, and files written for another group.

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
2. If a datapoint version is older and its ``allowUpgrade`` value is ``true``, a same-size payload is
   restored directly. A changed-size payload is passed to its model-declared migration callback.
3. If upgrade is not allowed, a datapoint version mismatch reports
   ``SerializationError::DatapointVersion`` (or ``GroupAndDatapointVersion``).
4. A group version mismatch is reported as ``SerializationError::GroupVersion`` when group upgrades
   are not allowed.

API Reference
-------------

.. doxygenstruct:: SerializationStatus
   :members:

.. doxygenenum:: SerializationError

.. doxygenstruct:: Serialization
   :members:

.. doxygenstruct:: Deserialization
   :members:
