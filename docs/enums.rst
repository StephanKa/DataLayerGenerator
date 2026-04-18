Enumerations
============

This page documents the enumerations provided by the C++ framework headers.  Enumerations defined in
your **model files** (``enums.json``) are generated into ``datalayer.h`` and — when ``ENABLE_FMT`` is
set — get ``fmt::formatter<>`` specializations in ``formatter.h``.

Framework Enumerations
----------------------

RangeCheck
~~~~~~~~~~

Returned by :cpp:func:`DataLayer::Detail::checkValue` and propagated through
:cpp:func:`DataLayer::DataPoint::set`, :cpp:func:`DataLayer::GroupDataPointMapping::setDatapoint`,
and :cpp:func:`DataLayer::Dispatcher::setDatapoint` to report the outcome of a range validation.

.. doxygenenum:: DataLayer::Detail::RangeCheck

PersistenceType
~~~~~~~~~~~~~~~

Specifies how an entire group is persisted.  Set in ``groups.json`` via the ``"persistence"`` key and
embedded as a ``constexpr`` field in :cpp:struct:`DataLayer::GroupInfo`.

.. doxygenenum:: DataLayer::PersistenceType

SerializationError
~~~~~~~~~~~~~~~~~~

Returned inside :cpp:struct:`SerializationStatus` by :cpp:func:`Deserialization::read` to describe
which version mismatch (if any) was detected during deserialization.

.. doxygenenum:: SerializationError

Model-Defined Enumerations
---------------------------

Enumerations defined in your ``enums.json`` model file are generated into ``datalayer.h``.
Two styles are supported:

**Auto-ID** — values are assigned sequentially starting at 0:

.. code-block:: json

   {
     "name": "Status",
     "type": "uint32_t",
     "values": ["Booting", "Starting", "Finished"],
     "autoId": true
   }

**Explicit values** — each entry is a ``{ "name": value }`` object:

.. code-block:: json

   {
     "name": "ErrorStates",
     "type": "uint32_t",
     "values": [
       { "None": 0 }, { "Info": 1 }, { "Warn": 4 },
       { "Error": 5 }, { "Critical": 9 }, { "Fatal": 10 }
     ],
     "autoId": false
   }

When ``ENABLE_FMT`` is set the generator outputs a ``fmt::formatter<ErrorStates>`` specialization in
``formatter.h`` so that enum values can be printed directly with ``fmt::print``.

See :doc:`model` for the full model file reference.
