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

TARGET_COMPILE_FEATURES(DataLayerGenerator INTERFACE cxx_std_20)

IF(ENABLE_FMT)
    TARGET_COMPILE_DEFINITIONS(DataLayerGenerator INTERFACE USE_FMT)
ENDIF()
IF(ENABLE_FILE_PERSISTENCE)
    TARGET_COMPILE_DEFINITIONS(DataLayerGenerator INTERFACE USE_FILE_PERSISTENCE)
ENDIF()

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
    IF(ENABLE_PYBIND11)
        LIST(APPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}")
        FIND_PACKAGE(Python COMPONENTS Interpreter Development REQUIRED)
        FIND_PACKAGE(pybind11 REQUIRED)

        SET_SOURCE_FILES_PROPERTIES(${CMAKE_BINARY_DIR}/generated/datalayer_example/pythonBinding.cpp PROPERTIES GENERATED TRUE)
        PYBIND11_ADD_MODULE(conan_pybind11 ${CMAKE_BINARY_DIR}/generated/datalayer_example/pythonBinding.cpp)
        TARGET_LINK_LIBRARIES(conan_pybind11
                              PRIVATE
                              project_warnings
                              DataLayerGenerator::DataLayerGenerator
                              fmt::fmt-header-only)
        TARGET_INCLUDE_DIRECTORIES(conan_pybind11 PRIVATE ${CMAKE_BINARY_DIR}/generated)
        ADD_DEPENDENCIES(conan_pybind11 GenerateDataLayer)
        ADD_CUSTOM_COMMAND(TARGET conan_pybind11 POST_BUILD
                           COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:conan_pybind11> ${CMAKE_BINARY_DIR}/install/bin/$<TARGET_FILE_NAME:conan_pybind11>
                           COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/generated ${CMAKE_BINARY_DIR}/install/generated
                           COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/../datalayer_example ${CMAKE_BINARY_DIR}/install/generated/datalayer_example
                           COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/generated/include ${CMAKE_BINARY_DIR}/install/generated/datalayer_example/include
                           COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_BINARY_DIR}/install/generated/datalayer_example/include)

        ADD_CUSTOM_COMMAND(
                TARGET conan_pybind11 POST_BUILD
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/install/generated/datalayer_example
                COMMAND ${CMAKE_COMMAND} -E tar "cvf" "${CMAKE_BINARY_DIR}/install/datalayer_example_pkg.zip" --format=zip .
        )
    ENDIF()
ENDIF()


SET(_GEN_STAMP "${CMAKE_BINARY_DIR}/generated/.datalayer_generated.stamp")

ADD_CUSTOM_COMMAND(
    OUTPUT  "${_GEN_STAMP}"
    COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generator/generator.py
            --model_dir    ${CMAKE_CURRENT_SOURCE_DIR}/model
            --out_dir      ${CMAKE_BINARY_DIR}
            --template_dir ${CMAKE_CURRENT_SOURCE_DIR}/template
            --schema_dir   ${CMAKE_CURRENT_SOURCE_DIR}/generator
            --module_name  conan_pybind11
    COMMAND ${CMAKE_COMMAND} -E touch "${_GEN_STAMP}"
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/model
    COMMENT "DataLayerGenerator: generating datalayer.h"
    VERBATIM
)

ADD_CUSTOM_TARGET(GenerateDataLayer DEPENDS "${_GEN_STAMP}")

TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/generated)
ADD_DEPENDENCIES(${PROJECT_NAME} GenerateDataLayer)
