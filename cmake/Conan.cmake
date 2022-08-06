MACRO(RUN_CONAN)
    # Download automatically, you can also just copy the conan.cmake file
    IF(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
        MESSAGE(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
        FILE(DOWNLOAD "https://github.com/conan-io/cmake-conan/raw/0.18.1/conan.cmake" "${CMAKE_BINARY_DIR}/conan.cmake" TLS_VERIFY ON)
    ENDIF()

    INCLUDE(${CMAKE_BINARY_DIR}/conan.cmake)

    CONAN_CMAKE_RUN(
            REQUIRES
            ${CONAN_EXTRA_REQUIRES}
            catch2/2.13.9
            fmt/8.1.1
            spdlog/1.10.0
            OPTIONS
            ${CONAN_EXTRA_OPTIONS}
            BASIC_SETUP
            CMAKE_TARGETS # individual targets to link to
            BUILD
            missing)
ENDMACRO()
