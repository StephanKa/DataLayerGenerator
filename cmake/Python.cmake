FIND_PACKAGE(Python COMPONENTS Interpreter REQUIRED)
FIND_PROGRAM(UV_EXECUTABLE NAMES uv REQUIRED)

IF(ENABLE_VENV)
    EXECUTE_PROCESS(COMMAND ${UV_EXECUTABLE} venv --python ${Python_EXECUTABLE} ${CMAKE_BINARY_DIR}/.venv)
    ## update the environment with VIRTUAL_ENV variable (mimic the activate script)
    SET(ENV{VIRTUAL_ENV} ${CMAKE_BINARY_DIR}/.venv)
    ## change the context of the search
    SET(Python_FIND_VIRTUALENV FIRST)
    ## unset Python_EXECUTABLE because it is also an input variable
    UNSET(Python_EXECUTABLE)
    ## Launch a new search
    FIND_PACKAGE(Python COMPONENTS Interpreter Development REQUIRED)
ENDIF()

# Install generator requirements only when requirements.txt changes.
SET(_UV_STAMP "${CMAKE_BINARY_DIR}/.uv_requirements.stamp")
SET(_UV_REQUIREMENTS "${CMAKE_CURRENT_SOURCE_DIR}/src/generator/requirements.txt")
IF(NOT EXISTS "${_UV_STAMP}" OR "${_UV_REQUIREMENTS}" IS_NEWER_THAN "${_UV_STAMP}")
    IF(ENABLE_VENV)
        EXECUTE_PROCESS(COMMAND ${UV_EXECUTABLE} pip install --python ${Python_EXECUTABLE} --upgrade -r "${_UV_REQUIREMENTS}")
    ELSE()
        EXECUTE_PROCESS(COMMAND ${UV_EXECUTABLE} pip install --python ${Python_EXECUTABLE} --user --upgrade -r "${_UV_REQUIREMENTS}")
    ENDIF()
    FILE(TOUCH "${_UV_STAMP}")
ENDIF()
UNSET(_UV_STAMP)
UNSET(_UV_REQUIREMENTS)
