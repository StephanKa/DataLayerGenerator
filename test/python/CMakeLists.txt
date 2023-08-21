ADD_CUSTOM_TARGET(python_tests COMMAND ${CMAKE_CTEST_COMMAND})

FOREACH(TEST_NAME testEnumValidator testStructValidator testGroupValidator testDatapointValidator)
    ADD_TEST(NAME python-${TEST_NAME}
             COMMAND ${Python_EXECUTABLE} ${TEST_NAME}.py
             WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/test/python)


    ADD_CUSTOM_TARGET(copy_python_${TEST_NAME}.py ALL
                      COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/test/python/${TEST_NAME}.py ${CMAKE_BINARY_DIR}/install/test/${TEST_NAME}.py)

ENDFOREACH()
