# DataLayerGeneratorHelpers.cmake
#
# Injected as a CMake build module by the Conan package.
# Provides the generate_datalayer() function for consumers.
#
# This file is installed at:
#   <package>/lib/cmake/DataLayerGenerator/DataLayerGeneratorHelpers.cmake
# So CMAKE_CURRENT_LIST_DIR points there, and "../../.." resolves to the package root.

get_filename_component(_DATALAYER_GENERATOR_PACKAGE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)

set(DATALAYER_GENERATOR_DIR      "${_DATALAYER_GENERATOR_PACKAGE_ROOT}/res/generator" CACHE PATH
    "Path to the DataLayerGenerator Python generator script directory")
set(DATALAYER_GENERATOR_TEMPLATE_DIR "${_DATALAYER_GENERATOR_PACKAGE_ROOT}/res/template" CACHE PATH
    "Path to the DataLayerGenerator Jinja2 template directory")

# ---------------------------------------------------------------------------
# generate_datalayer(<target>
#     MODEL_DIR   <path>          # directory containing groups/datapoints/... JSON or YAML files
#     OUT_DIR     <path>          # build-time output directory (default: CMAKE_CURRENT_BINARY_DIR)
#     [MODULE_NAME <name>]        # pybind11 module name (default: datalayer)
# )
#
# Creates a custom target <target>_GenerateDataLayer that runs generator.py with the
# given model directory and wires it as a dependency of <target>.
# Adds the generated include directory to the target's include path.
# ---------------------------------------------------------------------------
function(generate_datalayer TARGET_NAME)
    cmake_parse_arguments(
        ARG
        ""                          # flags
        "MODEL_DIR;OUT_DIR;MODULE_NAME"  # single-value args
        ""                          # multi-value args
        ${ARGN}
    )

    if(NOT ARG_MODEL_DIR)
        message(FATAL_ERROR "generate_datalayer: MODEL_DIR is required")
    endif()

    if(NOT ARG_OUT_DIR)
        set(ARG_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")
    endif()

    if(NOT ARG_MODULE_NAME)
        set(ARG_MODULE_NAME "datalayer")
    endif()

    find_package(Python COMPONENTS Interpreter REQUIRED)

    set(_GEN_STAMP "${ARG_OUT_DIR}/generated/.datalayer_generated.stamp")

    add_custom_command(
        OUTPUT  "${_GEN_STAMP}"
        COMMAND "${Python_EXECUTABLE}"
                "${DATALAYER_GENERATOR_DIR}/generator.py"
                --model_dir    "${ARG_MODEL_DIR}"
                --out_dir      "${ARG_OUT_DIR}"
                --template_dir "${DATALAYER_GENERATOR_TEMPLATE_DIR}"
                --schema_dir   "${DATALAYER_GENERATOR_DIR}"
                --module_name  "${ARG_MODULE_NAME}"
        COMMAND "${CMAKE_COMMAND}" -E touch "${_GEN_STAMP}"
        DEPENDS "${ARG_MODEL_DIR}"
        COMMENT "DataLayerGenerator: generating datalayer.h from ${ARG_MODEL_DIR}"
        VERBATIM
    )

    add_custom_target(
        ${TARGET_NAME}_GenerateDataLayer
        DEPENDS "${_GEN_STAMP}"
    )

    add_dependencies(${TARGET_NAME} ${TARGET_NAME}_GenerateDataLayer)

    target_include_directories(${TARGET_NAME}
        PRIVATE
            "${ARG_OUT_DIR}/generated/include"
            "${ARG_OUT_DIR}/generated"
    )
endfunction()
