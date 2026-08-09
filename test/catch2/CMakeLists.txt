FIND_PACKAGE(Catch2 REQUIRED)
FIND_PACKAGE(fmt REQUIRED)

INCLUDE(CTest)
INCLUDE(Catch)

FILE(GLOB _TEST_GENERATOR_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../../src/generator/*.py")
FILE(GLOB _TEST_TEMPLATE_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../../src/template/*.jinja2")

FOREACH(TEST_NAME datapoints groups enums)
    ADD_EXECUTABLE(${TEST_NAME}_tests ${TEST_NAME}/test.cpp modelTests.cpp)

    SET(_GEN_STAMP "${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}/.datalayer_generated.stamp")
        FILE(GLOB _TEST_MODEL_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_NAME}/model/*")
    ADD_CUSTOM_COMMAND(
        OUTPUT  "${_GEN_STAMP}"
        COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/../../src/generator/generator.py
                --model_dir ${CMAKE_CURRENT_SOURCE_DIR}/${TEST_NAME}/model
                --out_dir ${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}
                --template_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../src/template
                --schema_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../src/generator
        COMMAND ${CMAKE_COMMAND} -E touch "${_GEN_STAMP}"
        DEPENDS ${_TEST_MODEL_FILES} ${_TEST_GENERATOR_FILES} ${_TEST_TEMPLATE_FILES}
        COMMENT "DataLayerGenerator: generating test datalayer for ${TEST_NAME}"
        VERBATIM
    )
    ADD_CUSTOM_TARGET(${TEST_NAME}GenerateDataLayer DEPENDS "${_GEN_STAMP}")

    TARGET_INCLUDE_DIRECTORIES(${TEST_NAME}_tests PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}/generated ${CMAKE_CURRENT_SOURCE_DIR}/../../src/include)
    TARGET_LINK_LIBRARIES(${TEST_NAME}_tests PRIVATE project_warnings project_options Catch2::Catch2WithMain fmt::fmt-header-only)

    ADD_DEPENDENCIES(${TEST_NAME}_tests ${TEST_NAME}GenerateDataLayer)

    # automatically discover tests that are defined in catch based test files you can modify the unittests. Set TEST_PREFIX
    # to whatever you want, or use different for different binaries
    CATCH_DISCOVER_TESTS(
            ${TEST_NAME}_tests
            TEST_PREFIX
            "${TEST_NAME}."
            REPORTER
            JUnit
            OUTPUT_DIR
            .
            OUTPUT_PREFIX
            "${TEST_NAME}."
            OUTPUT_SUFFIX
            .xml)

    ADD_CUSTOM_COMMAND(TARGET ${TEST_NAME}_tests POST_BUILD
                       COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${TEST_NAME}_tests> ${CMAKE_BINARY_DIR}/install/test/$<TARGET_FILE_NAME:${TEST_NAME}_tests>)
ENDFOREACH()
