INCLUDE(CTest)
INCLUDE(Catch)

ADD_LIBRARY(catch_main STATIC catch_main.cpp)
TARGET_LINK_LIBRARIES(catch_main PUBLIC CONAN_PKG::catch2)
TARGET_LINK_LIBRARIES(catch_main PRIVATE project_options)

FOREACH(TEST_NAME datapoints groups enums)
    ADD_EXECUTABLE(${TEST_NAME}_tests ${TEST_NAME}/test.cpp)
    ADD_CUSTOM_TARGET(${TEST_NAME}GenerateDataLayer
                      COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/../../src/generator/generator.py
                      --model_dir ${CMAKE_CURRENT_SOURCE_DIR}/${TEST_NAME}/model
                      --out_dir ${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}
                      --template_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../src/template
                      --schema_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../src/generator)

    TARGET_INCLUDE_DIRECTORIES(${TEST_NAME}_tests PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}/generated ${CMAKE_CURRENT_SOURCE_DIR}/../../src/include)
    TARGET_LINK_LIBRARIES(${TEST_NAME}_tests PRIVATE project_warnings project_options catch_main CONAN_PKG::fmt)

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
ENDFOREACH()
