Code Generator
==============

The Python generator lives in ``src/generator/`` and is the entry point for turning model files into
C++ headers and Python bindings.

Requirements
------------

Python ≥ 3.9 and the following packages (also in ``src/generator/requirements.txt``):

.. code-block:: text

   jinja2
   jsonschema
   pyyaml

Install with:

.. code-block:: bash

   pip install -r src/generator/requirements.txt

Invocation
----------

.. code-block:: bash

   python src/generator/generator.py \
     --model_dir   src/model \
     --out_dir     build/ \
     --template_dir src/template \
     --schema_dir   src/generator \
     --module_name  my_module        # optional pybind11 module name

Arguments:

.. list-table::
   :header-rows: 1
   :widths: 20 80

   * - Argument
     - Description
   * - ``--model_dir``
     - Directory containing the JSON / YAML model files (``groups.json``, ``datapoints.json``, etc.)
   * - ``--out_dir``
     - Root output directory.  Generated files go into ``<out_dir>/generated/``.
   * - ``--template_dir``
     - Directory with the Jinja2 ``.jinja2`` template files.
   * - ``--schema_dir``
     - Directory containing ``schema.json`` for model validation.
   * - ``--module_name``
     - Name of the generated pybind11 Python module (default: ``datalayer_example``).
   * - ``--convert``
     - Convert JSON model files to YAML (Python ≥ 3.9: ``--convert`` / ``--no-convert``).

Output Files
------------

All files are written under ``<out_dir>/generated/``:

.. list-table::
   :header-rows: 1
   :widths: 35 65

   * - File
     - Content
   * - ``include/datalayer.h``
     - Main generated header — group infos, custom types, enums, structs, and all ``constinit``
       datapoint globals.
   * - ``include/formatter.h``
     - ``fmt::formatter<>`` specializations for all generated structs and enums
       (only when ``ENABLE_FMT`` is set).
   * - ``datalayer_example/pythonBinding.cpp``
     - pybind11 module source (only when ``ENABLE_PYBIND11`` is set and ``--module_name`` is given).
   * - ``doc/*.puml``
     - PlantUML class diagram sources for the generated data model.
   * - ``doc/overview.csv``
     - CSV listing all datapoints: ID, name, type, access, group, version.

CMake Integration
-----------------

The generator is wired into CMake via a custom target.  When building the project directly:

.. code-block:: cmake

   # In src/CMakeLists.txt (already set up)
   add_custom_command(OUTPUT "${_GEN_STAMP}" ...)
   add_custom_target(GenerateDataLayer DEPENDS "${_GEN_STAMP}")
   add_dependencies(DatalayerGeneratorExample GenerateDataLayer)

When consuming the library as a Conan package the ``generate_datalayer()`` CMake function from
``DataLayerGeneratorHelpers.cmake`` provides the same capability:

.. code-block:: cmake

   find_package(DataLayerGenerator REQUIRED)
   add_executable(my_app main.cpp)
   target_link_libraries(my_app PRIVATE DataLayerGenerator::DataLayerGenerator)
   generate_datalayer(my_app
       MODEL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/model
       OUT_DIR   ${CMAKE_CURRENT_BINARY_DIR}
   )

See :doc:`conan` for the full consumer workflow.

Validation
----------

The generator validates model files in two passes:

1. **JSON Schema validation** — ``schema.json`` checks structural correctness (required fields,
   allowed values for ``"access"``, ``"persistence"``, etc.).
2. **Semantic validation** — Python validators in ``validators.py`` check cross-references:

   - Every datapoint ``"group"`` must refer to a group defined in ``Groups``.
   - Every datapoint ``"type"`` must be a base type, a defined struct, enum, or alias type.
   - Duplicate names and duplicate IDs within a group are rejected.
   - Alias types with both ``"min"`` and ``"max"`` must satisfy ``min < max``.

Templates
---------

Three Jinja2 templates drive code generation:

.. list-table::
   :header-rows: 1
   :widths: 35 65

   * - Template
     - Output
   * - ``datalayer.h.jinja2``
     - ``include/datalayer.h`` — the main generated C++ header.
   * - ``customFormatter.h.jinja2``
     - ``include/formatter.h`` — ``fmt`` formatters for structs and enums.
   * - ``pythonBinding.jinja2``
     - ``datalayer_example/pythonBinding.cpp`` — pybind11 module source.
</content>
</invoke>
