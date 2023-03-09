MACRO(RUN_CONAN)
    # Download automatically, you can also just copy the conan.cmake file
    IF(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
        MESSAGE(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
        FILE(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/9445e140706012c218bb2283620bec950190359d/conan.cmake" "${CMAKE_BINARY_DIR}/conan.cmake" TLS_VERIFY ON)
    ENDIF()

    INCLUDE(${CMAKE_BINARY_DIR}/conan.cmake)

    SET(FMT_LIBRARY "")
    IF(ENABLE_FMT)
        SET(FMT_LIBRARY fmt/9.1.0)
    ENDIF()

    conan_cmake_configure(REQUIRES  ${CONAN_EXTRA_REQUIRES}
                                    catch2/3.2.1
                                    ${FMT_LIBRARY}
                          OPTIONS	${CONAN_EXTRA_OPTIONS}
                          SETTINGS  compiler.cppstd=${CMAKE_CXX_STANDARD}
                          GENERATORS cmake
            )

    conan_cmake_autodetect(settings)

    conan_cmake_install(PATH_OR_REFERENCE .
                        BUILD missing
                        SETTINGS ${settings}
                        CONF "tools.cmake.cmaketoolchain:generator=${CMAKE_GENERATOR}")

    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup(TARGETS)
ENDMACRO()
