Packaging & Integration
=======================

This page covers how to consume DataLayerGenerator as a **Conan package** and how to integrate the
code generator into your own CMake project.

Installing the Package from Source
------------------------------------

.. code-block:: bash

   # From the repository root
   conan create . --build=missing

This installs ``datalayer-generator/0.0.1`` into your local Conan cache.

Conan Options
-------------

.. list-table::
   :header-rows: 1
   :widths: 30 15 55

   * - Option
     - Default
     - Description
   * - ``with_fmt``
     - ``True``
     - Require ``fmt`` and propagate ``-DUSE_FMT`` to consumers.
   * - ``with_file_persistence``
     - ``False``
     - Propagate ``-DUSE_FILE_PERSISTENCE`` to consumers.
   * - ``with_pybind11``
     - ``False``
     - Require ``pybind11`` (only needed if you generate Python bindings).

Example ``conanfile.txt`` for a consumer project:

.. code-block:: ini

   [requires]
   datalayer-generator/0.0.1

   [options]
   datalayer-generator/*:with_fmt=True
   datalayer-generator/*:with_file_persistence=False

   [generators]
   CMakeDeps
   CMakeToolchain

CMake Integration for Consumers
---------------------------------

After running ``conan install`` the ``DataLayerGeneratorHelpers.cmake`` module is injected
automatically.  Use ``find_package`` and the ``generate_datalayer()`` function:

.. code-block:: cmake

   cmake_minimum_required(VERSION 3.19)
   project(MyApp LANGUAGES CXX)

   find_package(DataLayerGenerator REQUIRED)

   add_executable(my_app main.cpp)
   target_link_libraries(my_app PRIVATE DataLayerGenerator::DataLayerGenerator)

   # Run generator.py at build time from your own model directory
   generate_datalayer(my_app
       MODEL_DIR   ${CMAKE_CURRENT_SOURCE_DIR}/model
       OUT_DIR     ${CMAKE_CURRENT_BINARY_DIR}
       MODULE_NAME my_python_module   # optional, for pybind11
   )

The ``generate_datalayer()`` function:

- Finds the Python interpreter.
- Adds an ``add_custom_command`` that runs ``generator.py`` when model files change (stamp-file based
  — no unnecessary reruns).
- Adds ``<out_dir>/generated/include`` and ``<out_dir>/generated`` to the target's include path.

The ``DataLayerGenerator::DataLayerGenerator`` INTERFACE target:

- Adds ``src/include/`` (the framework headers) to the consumer's include path.
- Requires C++20.
- Propagates ``USE_FMT`` and/or ``USE_FILE_PERSISTENCE`` defines when the corresponding Conan options
  are enabled.

Building the Documentation
---------------------------

The docs target requires Doxygen and Sphinx.  Enable it by uncommenting
``ADD_SUBDIRECTORY(docs)`` in the root ``CMakeLists.txt`` and install the Python dependencies:

.. code-block:: bash

   pip install sphinx sphinx-rtd-theme breathe sphinx-copybutton

Then build:

.. code-block:: bash

   cmake --build build --target Sphinx

The HTML output is written to ``<build>/docs/sphinx/``.

Cross-Compilation (ARM Cortex-M4)
----------------------------------

The project includes a GCC ARM Cortex-M4 toolchain in ``cmake/arm-cortex-gnu/``.  When
``CMAKE_CROSSCOMPILING`` is set (e.g. via ``CMakePresets.json`` or a ``-DCMAKE_TOOLCHAIN_FILE``):

- The build switches to ``embedded.cpp`` as the entry point.
- The linker script ``src/linkerscript/linkerScript.ld`` is applied.
- ``.elf``, ``.bin``, and ``.hex`` artefacts are produced in ``install/``.
- Tests are skipped.
- Only ``fmt`` is required by Conan (``catch2`` and ``pybind11`` are excluded on ``armv7``).
</content>
</invoke>
