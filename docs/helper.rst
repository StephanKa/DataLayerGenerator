Helper Types
============

The ``helper.h`` header provides the fundamental building blocks used throughout the framework.
All types in this header have no runtime dependency beyond the C++20 standard library.

Version
-------

A plain ``constexpr``-capable struct that carries ``major``, ``minor``, and ``build`` as ``uint32_t``
fields.  It supports ``!=`` and ``>`` comparisons and is used in two places:

- As a **group version** embedded in :cpp:struct:`DataLayer::GroupInfo` — checked during deserialization
  to detect model changes.
- As a **datapoint version** on individual :cpp:struct:`DataLayer::DataPoint` instances — used to skip
  deserializing a single datapoint when only its version changed (when ``AllowUpgrade`` is set).

.. doxygenstruct:: Version
   :members:

FixedString
-----------

A compile-time string literal wrapper used as a Non-Type Template Parameter (NTTP).
It stores the characters in a ``std::array<char, N+1>`` and is implicitly convertible to
``std::string_view``.  Deduction guides handle the common ``FixedString("my_name")`` usage.

.. doxygenstruct:: FixedString
   :members:

PersistenceType
---------------

Controls how a group's data is persisted.  Set in ``groups.json`` and propagated into
:cpp:struct:`DataLayer::GroupInfo`.

.. doxygenenum:: DataLayer::PersistenceType

Access Tag Types
----------------

Three empty tag structs used as the ``Access`` template parameter of
:cpp:struct:`DataLayer::DataPoint`.  Their presence or absence controls which ``get()`` and ``set()``
overloads are available (enforced by C++20 Concepts — see :doc:`concepts`).

.. doxygenstruct:: Helper::READ_ONLY
.. doxygenstruct:: Helper::WRITE_ONLY
.. doxygenstruct:: Helper::READ_WRITE

BaseType
--------

A thin wrapper around a scalar type ``T`` used to implement **alias types with optional range bounds**.
When ``Minimum`` and ``Maximum`` static members are present, :cpp:func:`DataLayer::Detail::checkValue`
enforces the range before a value is stored.

.. doxygenstruct:: DataLayer::Detail::BaseType
   :members:

CheckResult
-----------

Returned by :cpp:func:`DataLayer::GroupDataPointMapping::setDatapoint` and
:cpp:func:`DataLayer::Dispatcher::setDatapoint` to communicate whether the write succeeded and whether
a range check fired.

.. doxygenstruct:: DataLayer::Detail::CheckResult
   :members:

Helper Utilities
----------------

.. doxygenfunction:: DataLayer::Detail::make_array
.. doxygenfunction:: DataLayer::Detail::checkValue
