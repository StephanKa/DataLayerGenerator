INCLUDE(CTest)
INCLUDE(Catch)

ADD_LIBRARY(catch_main STATIC catch_main.cpp)
TARGET_LINK_LIBRARIES(catch_main PUBLIC CONAN_PKG::catch2)
TARGET_LINK_LIBRARIES(catch_main PRIVATE project_options)

ADD_EXECUTABLE(catch_tests tests.cpp)
TARGET_LINK_LIBRARIES(catch_tests PRIVATE project_warnings project_options catch_main)

# automatically discover tests that are defined in catch based test files you can modify the unittests. Set TEST_PREFIX
# to whatever you want, or use different for different binaries
CATCH_DISCOVER_TESTS(
        catch_tests
        TEST_PREFIX
        "catch_unittests."
        REPORTER
        JUnit
        OUTPUT_DIR
        .
        OUTPUT_PREFIX
        "catch_unittests."
        OUTPUT_SUFFIX
        .xml)
