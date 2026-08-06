SEGGER RTT Monitoring
=====================

The generator creates ``generated/datalayer_example/rtt.py``: a Python reader for values published by
an embedded target through SEGGER RTT. It uses the model's stable *global datapoint IDs* (group base ID
+ datapoint ID), rather than object addresses. This remains valid when the linker layout changes.

The generated reader is optional. The C++ framework has no PyLink or SEGGER dependency.

Protocol
--------

The target writes one little-endian binary frame per value to an RTT up-channel. A frame has the
following ``<2sBBHH`` layout, followed by the payload:

.. list-table::
   :header-rows: 1

   * - Field
     - Size
     - Value
   * - Magic
     - 2 bytes
     - ASCII ``DL``
   * - Protocol version
     - 1 byte
     - ``1``
   * - Flags
     - 1 byte
     - ``0`` (reserved)
   * - Datapoint ID
     - 2 bytes
     - Group base ID + datapoint ID
   * - Payload length
     - 2 bytes
     - Number of following bytes
   * - Payload
     - variable
     - Native little-endian scalar value or scalar array

The payload must match the C++ scalar's byte representation. The generated reader supports the integer,
floating-point, and ``bool`` base types, plus enum and alias types based on them. Structs and
``std::string_view`` values are returned as raw ``bytes`` because their in-memory ABI is target-specific.

.. mermaid::

   flowchart LR
       DP[Generated DataPoint] --> PUB[Application publisher]
       PUB --> RTT[SEGGER RTT up-channel]
       RTT --> JL[J-Link probe]
       JL --> PY[Generated rtt.py via PyLink]
       PY --> VALUE[Decoded name and value]

Target Integration
------------------

Configure and initialize SEGGER RTT in the target application as usual. At the desired sampling point
or from a datapoint change callback, write a header and value payload to the same RTT up-channel. The
following example publishes an ``int32_t`` datapoint with global ID ``0x4001``:

.. code-block:: cpp

   #include <SEGGER_RTT.h>
   #include <array>
   #include <bit>
   #include <cstdint>

   struct RttFrameHeader
   {
       std::array<char, 2> magic{ 'D', 'L' };
       uint8_t version{ 1 };
       uint8_t flags{ 0 };
       uint16_t datapointId{ 0x4001 };
       uint16_t payloadSize{ sizeof(int32_t) };
   };

   void publishTest(int32_t value)
   {
       static_assert(std::endian::native == std::endian::little);
       const RttFrameHeader header{};
       SEGGER_RTT_Write(0, &header, sizeof(header));
       SEGGER_RTT_Write(0, &value, sizeof(value));
   }

Ensure the header uses the exact packed byte layout above. A manual byte serialization is preferable to
writing a C++ struct directly unless the target ABI and packing are explicitly controlled.

Host Usage
----------

Install PyLink in the host environment:

.. code-block:: bash

   python -m pip install pylink-square

Then create the generated reader with the target device name and consume available frames:

.. code-block:: python

   from rtt import RttReader

   reader = RttReader.connect(device='STM32F407VG', speed_khz=4000, channel=0)
   for name, value in reader.read():
       print(f'{name}: {value}')

``RttReader.read()`` is non-blocking with respect to framing: it retains partial RTT data internally and
returns only complete frames. Unknown IDs are skipped, allowing the host to tolerate target-side
datapoints not present in its generated model.
