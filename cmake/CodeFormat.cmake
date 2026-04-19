# To exclude directories from the format check, add corresponding clang-format config files into those directories.
FIND_PROGRAM(CLANG_FORMAT_BINARY NAMES clang-format REQUIRED)
FIND_PROGRAM(CLANG_TIDY_BINARY   NAMES clang-tidy   REQUIRED)

ADD_CUSTOM_TARGET(clang-format-check
                  USES_TERMINAL
                  COMMAND ${Python_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/run-clang-format.py
                          -clang-format-binary ${CLANG_FORMAT_BINARY}
                          -warnings-as-errors
                  )

ADD_CUSTOM_TARGET(clang-format-fix
                  USES_TERMINAL
                  COMMAND ${Python_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/run-clang-format.py
                          -clang-format-binary ${CLANG_FORMAT_BINARY}
                          -fix
                  )

ADD_CUSTOM_TARGET(clang-tidy-check
                  USES_TERMINAL
                  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                  COMMAND ${Python_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/run-clang-tidy.py
                          -clang-tidy-binary ${CLANG_TIDY_BINARY}
                          -p ${CMAKE_BINARY_DIR}
)

ADD_CUSTOM_TARGET(clang-tidy-diff-check
                  USES_TERMINAL
                  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                  COMMAND git diff -U0 HEAD --no-prefix | ${Python_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/run-clang-tidy-diff.py
                          -clang-tidy-binary ${CLANG_TIDY_BINARY}
                          -path ${CMAKE_BINARY_DIR}
)
