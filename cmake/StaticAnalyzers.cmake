IF(ENABLE_CPPCHECK)
    FIND_PROGRAM(CPPCHECK cppcheck)
    IF(CPPCHECK)
        SET(CMAKE_CXX_CPPCHECK
            ${CPPCHECK}
            --suppress=missingInclude
            --enable=all
            --inline-suppr
            --inconclusive)
    ELSE()
        MESSAGE(SEND_ERROR "cppcheck requested but executable not found")
    ENDIF()
ENDIF()

IF(ENABLE_CLANG_TIDY)
    FIND_PROGRAM(CLANG_TIDY_BINARY clang-tidy)
    IF(CLANG_TIDY_BINARY)
        SET(CMAKE_CXX_CLANG_TIDY
            ${CLANG_TIDY_BINARY}
            -extra-arg=-Wno-unknown-warning-option
            -p=${CMAKE_BINARY_DIR})
    ELSE()
        MESSAGE(SEND_ERROR "clang-tidy requested but executable not found")
    ENDIF()
ENDIF()

IF(ENABLE_INCLUDE_WHAT_YOU_USE)
    FIND_PROGRAM(INCLUDE_WHAT_YOU_USE include-what-you-use)
    IF(INCLUDE_WHAT_YOU_USE)
        SET(CMAKE_CXX_INCLUDE_WHAT_YOU_USE ${INCLUDE_WHAT_YOU_USE})
    ELSE()
        MESSAGE(SEND_ERROR "include-what-you-use requested but executable not found")
    ENDIF()
ENDIF()
