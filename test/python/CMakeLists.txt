ADD_TEST(NAME python-testEnumValidator
         COMMAND ${Python_EXECUTABLE} testEnumValidator.py
         WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/test/python)

ADD_TEST(NAME python-testStructValidator
         COMMAND ${Python_EXECUTABLE} testStructValidator.py
         WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/test/python)

ADD_TEST(NAME python-testGroupValidator
         COMMAND ${Python_EXECUTABLE} testGroupValidator.py
         WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/test/python)

ADD_TEST(NAME python-testDatapointValidator
         COMMAND ${Python_EXECUTABLE} testDatapointValidator.py
         WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/test/python)
