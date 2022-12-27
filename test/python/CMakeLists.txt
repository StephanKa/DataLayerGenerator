FOREACH(TEST_NAME testEnumValidator testStructValidator testGroupValidator testDatapointValidator)
    ADD_TEST(NAME python-${TEST_NAME}
             COMMAND ${Python_EXECUTABLE} ${TEST_NAME}.py
             WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/test/python)
ENDFOREACH()
