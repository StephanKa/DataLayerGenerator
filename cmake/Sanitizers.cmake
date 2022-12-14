FUNCTION(ENABLE_SANITIZERS project_name)

    IF(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")

        IF(ENABLE_COVERAGE)
            TARGET_COMPILE_OPTIONS(${project_name} INTERFACE --coverage -O0 -g)
            TARGET_LINK_LIBRARIES(${project_name} INTERFACE --coverage)
        ENDIF()

        SET(SANITIZERS "")

        IF(ENABLE_SANITIZER_ADDRESS)
            LIST(APPEND SANITIZERS "address")
        ENDIF()

        IF(ENABLE_SANITIZER_LEAK)
            LIST(APPEND SANITIZERS "leak")
        ENDIF()

        IF(ENABLE_SANITIZER_UNDEFINED_BEHAVIOR)
            LIST(APPEND SANITIZERS "undefined")
        ENDIF()

        IF(ENABLE_SANITIZER_THREAD)
            IF("address" IN_LIST SANITIZERS OR "leak" IN_LIST SANITIZERS)
                MESSAGE(WARNING "Thread sanitizer does not work with Address and Leak sanitizer enabled")
            ELSE()
                LIST(APPEND SANITIZERS "thread")
            ENDIF()
        ENDIF()

        IF(ENABLE_SANITIZER_MEMORY AND CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
            IF("address" IN_LIST SANITIZERS
               OR "thread" IN_LIST SANITIZERS
               OR "leak" IN_LIST SANITIZERS)
                MESSAGE(WARNING "Memory sanitizer does not work with Address, Thread and Leak sanitizer enabled")
            ELSE()
                LIST(APPEND SANITIZERS "memory")
            ENDIF()
        ENDIF()

        LIST(
                JOIN
                SANITIZERS
                ","
                LIST_OF_SANITIZERS)

    ENDIF()

    IF(LIST_OF_SANITIZERS)
        IF(NOT
           "${LIST_OF_SANITIZERS}"
           STREQUAL
           "")
            TARGET_COMPILE_OPTIONS(${project_name} INTERFACE -fsanitize=${LIST_OF_SANITIZERS})
            TARGET_LINK_OPTIONS(${project_name} INTERFACE -fsanitize=${LIST_OF_SANITIZERS})
        ENDIF()
    ENDIF()

ENDFUNCTION()
