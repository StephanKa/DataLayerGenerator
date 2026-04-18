C++20 Concepts
==============

The framework uses C++20 Concepts to enforce access control and type constraints at compile time.
Violating a concept (e.g. calling ``get()`` on a ``WRITE_ONLY`` datapoint) produces a clear
compiler error at the call site rather than a runtime failure.

Access Concepts
---------------

These two concepts gate the ``get()`` and ``set()`` member functions of
:cpp:struct:`DataLayer::DataPoint`.

.. doxygenconcept:: DataLayer::Helper::ReadConcept

.. doxygenconcept:: DataLayer::Helper::WriteConcept

Usage example:

.. code-block:: cpp

   // READ_WRITE datapoint — both get() and set() are available
   constinit DataLayer::DataPoint<int32_t, MyGroup, 1, Helper::READ_WRITE> myPoint;
   myPoint.set(42);          // OK
   auto val = myPoint.get(); // OK

   // READ_ONLY datapoint — set() is not instantiated
   constinit DataLayer::DataPoint<int32_t, MyGroup, 2, Helper::READ_ONLY> roPoint;
   // roPoint.set(42);       // compile error: WriteConcept not satisfied

Type Concepts
-------------

Used internally by :cpp:struct:`DataLayer::DataPoint` and the serialization layer to branch on
container vs. scalar types.

.. doxygenconcept:: DataLayer::Detail::IsArray

Used by :cpp:struct:`Serialization` and :cpp:struct:`Deserialization` to handle ``std::string``
and ``std::string_view`` fields differently from fixed-size containers.

.. doxygenconcept:: IsContainer
.. doxygenconcept:: IsString

Range Concept
-------------

Used internally by :cpp:func:`DataLayer::Detail::checkValue` to detect whether a type carries
``Minimum`` / ``Maximum`` static members (alias types defined in ``types.json``).

.. doxygenconcept:: DataLayer::Detail::hasRange
