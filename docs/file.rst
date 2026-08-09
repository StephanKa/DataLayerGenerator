File Persistence (Serialization)
================================

When the project is built with ``-DENABLE_FILE_PERSISTENCE=ON`` (compiles with ``-DUSE_FILE_PERSISTENCE``),
the serialization layer in ``serialization.h`` is available.  It provides binary file I/O for entire
groups with group- and datapoint-level version validation.

Format and Integrity
--------------------

Persistence writes the ``DLG1`` format only. Each record set has a magic value, format version, group
ID, group version, and CRC32 checksum. Each datapoint is stored as an ID-tagged, versioned,
length-delimited record. Unknown records can therefore be skipped when a newer model adds fields.
The built-in file backend stages data in a ``.tmp`` file before replacing the target file.

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

Custom Storage Backends
-----------------------

``serializeGroup`` and ``deserializeGroup`` also accept a caller-owned storage backend. This retains
the same ``DLG1`` bytes and validation behavior while allowing EEPROM, flash-slot, or application
storage implementations. A backend must provide these methods:

.. code-block:: cpp

   struct StorageBackend
   {
       bool write(std::span<const std::byte> bytes);
       std::optional<size_t> size() const;
       bool read(std::span<std::byte> destination) const;
   };

``write`` should make the supplied record set available atomically according to the backend's medium.
For example, a flash backend can commit to an inactive slot after programming and verification. The
framework does not select erase geometry, wear leveling, or power-failure policy for the application.

.. code-block:: cpp

   EepromBackend eeprom{ /* application-owned device and address */ };
   auto written = CyclicGroup.serializeGroup(eeprom);
   auto restored = CyclicGroup.deserializeGroup(eeprom);

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

.. doxygenclass:: DataLayer::Persistence::FileBackend
   :members:
