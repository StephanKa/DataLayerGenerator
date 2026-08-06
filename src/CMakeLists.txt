# ---------------------------------------------------------------------------
# DataLayerGenerator — header-only INTERFACE library
# Consumers link against this target to get the include path and compile
# definitions. The actual code-generated header (datalayer.h) is added via
# the generate_datalayer() CMake function from DataLayerGeneratorHelpers.cmake.
# ---------------------------------------------------------------------------
ADD_LIBRARY(DataLayerGenerator INTERFACE)
ADD_LIBRARY(DataLayerGenerator::DataLayerGenerator ALIAS DataLayerGenerator)

TARGET_INCLUDE_DIRECTORIES(DataLayerGenerator
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

TARGET_COMPILE_FEATURES(DataLayerGenerator INTERFACE cxx_std_23)

IF(ENABLE_FMT)
    FIND_PACKAGE(fmt REQUIRED)
    TARGET_COMPILE_DEFINITIONS(DataLayerGenerator INTERFACE USE_FMT)
    TARGET_LINK_LIBRARIES(DataLayerGenerator INTERFACE fmt::fmt-header-only)
ENDIF()
IF(ENABLE_FILE_PERSISTENCE)
    TARGET_COMPILE_DEFINITIONS(DataLayerGenerator INTERFACE USE_FILE_PERSISTENCE)
ENDIF()

SET(PYBIND11_MODULE_NAME "datalayer_example" CACHE STRING "Name of the generated pybind11 Python module")

FILE(GLOB_RECURSE _DATALAYER_MODEL_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/model/*")
FILE(GLOB _DATALAYER_GENERATOR_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/generator/*.py")
FILE(GLOB _DATALAYER_TEMPLATE_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/template/*.jinja2")
SET(_GEN_STAMP "${CMAKE_BINARY_DIR}/generated/.datalayer_generated.stamp")

ADD_CUSTOM_COMMAND(
    OUTPUT  "${_GEN_STAMP}"
    COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generator/generator.py
            --model_dir    ${CMAKE_CURRENT_SOURCE_DIR}/model
            --out_dir      ${CMAKE_BINARY_DIR}
            --template_dir ${CMAKE_CURRENT_SOURCE_DIR}/template
            --schema_dir   ${CMAKE_CURRENT_SOURCE_DIR}/generator
            --module_name  ${PYBIND11_MODULE_NAME}
    COMMAND ${CMAKE_COMMAND} -E touch "${_GEN_STAMP}"
    DEPENDS ${_DATALAYER_MODEL_FILES} ${_DATALAYER_GENERATOR_FILES} ${_DATALAYER_TEMPLATE_FILES}
    COMMENT "DataLayerGenerator: generating datalayer.h"
    VERBATIM
)

ADD_CUSTOM_TARGET(GenerateDataLayer DEPENDS "${_GEN_STAMP}")

IF(ENABLE_PYBIND11 AND NOT CMAKE_CROSSCOMPILING)
    FIND_PACKAGE(Python COMPONENTS Interpreter Development.Module REQUIRED)
    FIND_PACKAGE(pybind11 CONFIG QUIET)
    IF(NOT pybind11_FOUND)
        EXECUTE_PROCESS(
            COMMAND ${Python_EXECUTABLE} -m pybind11 --cmakedir
            OUTPUT_VARIABLE _PYBIND11_CMAKE_DIR
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE _PYBIND11_CMAKE_DIR_RESULT
        )
        IF(_PYBIND11_CMAKE_DIR_RESULT EQUAL 0)
            LIST(PREPEND CMAKE_PREFIX_PATH "${_PYBIND11_CMAKE_DIR}")
        ENDIF()
        FIND_PACKAGE(pybind11 CONFIG REQUIRED)
    ENDIF()

    SET_SOURCE_FILES_PROPERTIES(${CMAKE_BINARY_DIR}/generated/datalayer_example/pythonBinding.cpp PROPERTIES GENERATED TRUE)
    IF(PYBIND11_DISABLE_EXTRAS)
        PYBIND11_ADD_MODULE(${PYBIND11_MODULE_NAME} NO_EXTRAS ${CMAKE_BINARY_DIR}/generated/datalayer_example/pythonBinding.cpp)
    ELSE()
        PYBIND11_ADD_MODULE(${PYBIND11_MODULE_NAME} ${CMAKE_BINARY_DIR}/generated/datalayer_example/pythonBinding.cpp)
    ENDIF()
    TARGET_LINK_LIBRARIES(${PYBIND11_MODULE_NAME}
                          PRIVATE
                          project_warnings
                          DataLayerGenerator::DataLayerGenerator)
    TARGET_INCLUDE_DIRECTORIES(${PYBIND11_MODULE_NAME} PRIVATE ${CMAKE_BINARY_DIR}/generated)
    ADD_DEPENDENCIES(${PYBIND11_MODULE_NAME} GenerateDataLayer)
    INSTALL(TARGETS ${PYBIND11_MODULE_NAME}
            LIBRARY DESTINATION .
            RUNTIME DESTINATION .)
ENDIF()

IF(BUILD_EXAMPLES)
# ---------------------------------------------------------------------------
# Example executable
# ---------------------------------------------------------------------------
SET(PROJECT_NAME DatalayerGeneratorExample)

IF(CMAKE_CROSSCOMPILING)
    ADD_EXECUTABLE(${PROJECT_NAME} embedded.cpp)
    TARGET_LINK_LIBRARIES(
            ${PROJECT_NAME}
            PRIVATE project_options
            project_warnings
            DataLayerGenerator::DataLayerGenerator)

    SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES SUFFIX ".elf")
    TARGET_LINK_OPTIONS(${PROJECT_NAME}
                        PRIVATE
                        -T${CMAKE_CURRENT_SOURCE_DIR}/linkerscript/linkerScript.ld
                        -Wl,-Map=${PROJECT_NAME}.map
    )

    ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD
                       COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/generated ${CMAKE_BINARY_DIR}/install/generated)

    ADD_CUSTOM_TARGET(${PROJECT_NAME}.bin ALL
                      DEPENDS ${PROJECT_NAME}
                      COMMAND ${CMAKE_OBJCOPY} -Obinary ${CMAKE_BINARY_DIR}/src/${PROJECT_NAME}.elf ${CMAKE_BINARY_DIR}/install/${PROJECT_NAME}.bin
                      COMMAND ${CMAKE_OBJCOPY} -Oihex ${CMAKE_BINARY_DIR}/src/${PROJECT_NAME}.elf ${CMAKE_BINARY_DIR}/install/${PROJECT_NAME}.hex
    )

ELSE()
    FIND_PACKAGE(fmt REQUIRED)

    ADD_EXECUTABLE(${PROJECT_NAME} main.cpp)
    TARGET_LINK_LIBRARIES(
            ${PROJECT_NAME}
            PRIVATE project_options
            project_warnings
            DataLayerGenerator::DataLayerGenerator
            fmt::fmt-header-only)

    ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD
                       COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ${CMAKE_BINARY_DIR}/install/bin/$<TARGET_FILE_NAME:${PROJECT_NAME}>)

    ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD
                       COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/generated ${CMAKE_BINARY_DIR}/install/generated)
ENDIF()

TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/generated)
ADD_DEPENDENCIES(${PROJECT_NAME} GenerateDataLayer)
ENDIF()
