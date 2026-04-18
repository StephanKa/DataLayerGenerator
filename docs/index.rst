DataLayerGenerator
==================

A compile-time C++20 data layer framework with Python/Jinja2 code generation for embedded and host applications.

The library is header-only. You describe your data model (datapoints, groups, types, enums, structs) in
JSON or YAML files, run the generator, and get a fully type-safe ``datalayer.h`` header with ``constinit``
objects, range-checked access, optional persistence, optional ``fmt`` formatters, and optional Python bindings.

.. toctree::
   :maxdepth: 2
   :caption: Getting Started

   basic.rst

.. toctree::
   :maxdepth: 2
   :caption: Model File Reference

   model.rst

.. toctree::
   :maxdepth: 2
   :caption: Code Generator

   generator.rst

.. toctree::
   :maxdepth: 2
   :caption: C++ API Reference

   helper.rst
   concepts.rst
   enums.rst
   file.rst

.. toctree::
   :maxdepth: 2
   :caption: Packaging & Integration

   conan.rst

.. toctree::
   :maxdepth: 2
   :caption: Examples

   example.rst
