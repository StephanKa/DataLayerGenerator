# misc
OPTION(ENABLE_CACHE "Enable cache if available" ON)
OPTION(WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)
OPTION(ENABLE_DOXYGEN "Enable doxygen doc builds of source" OFF)
OPTION(ENABLE_COVERAGE "Enable coverage reporting for gcc/clang" OFF)
OPTION(ENABLE_FMT "Enable fmt formatting" OFF)
OPTION(BUILD_SHARED_LIBS "Enable compilation of shared libraries" OFF)
OPTION(ENABLE_TESTING "Enable Test Builds" ON)
OPTION(ENABLE_DOCS "Enable Sphinx/Doxygen documentation build" OFF)
OPTION(DOCS_ONLY "Skip all C++ targets and Conan — build documentation only" OFF)
OPTION(ENABLE_FILE_PERSISTENCE "Enable binary file persistence (serialization)" OFF)
OPTION(ENABLE_FUZZING "Enable Fuzzing Builds" OFF)
OPTION(ENABLE_PYBIND11 "Enable pybind11 binding generation" OFF)
OPTION(ENABLE_VENV "Enable python virtualenv" OFF)

# sanitizer options
OPTION(ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" OFF)
OPTION(ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
OPTION(ENABLE_SANITIZER_UNDEFINED_BEHAVIOR "Enable undefined behavior sanitizer" OFF)
OPTION(ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
OPTION(ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF)

# link time option
OPTION(ENABLE_IPO "Enable Interprocedural Optimization, aka Link Time Optimization (LTO)" OFF)

# static analyzers options
OPTION(ENABLE_CPPCHECK "Enable static analysis with cppcheck" OFF)
OPTION(ENABLE_CLANG_TIDY "Enable static analysis with clang-tidy" OFF)
OPTION(ENABLE_INCLUDE_WHAT_YOU_USE "Enable static analysis with include-what-you-use" OFF)

# Very basic PCH example
OPTION(ENABLE_PCH "Enable Precompiled Headers" OFF)
