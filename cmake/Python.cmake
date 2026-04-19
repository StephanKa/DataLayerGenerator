FIND_PACKAGE(Python COMPONENTS Interpreter REQUIRED)

IF(ENABLE_VENV)
    # install PyPI Python package using pip
    EXECUTE_PROCESS(COMMAND ${Python_EXECUTABLE} -m pip install --upgrade pip virtualenv)

    EXECUTE_PROCESS(COMMAND ${Python_EXECUTABLE} -m venv ${CMAKE_BINARY_DIR}/.venv)
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
SET(_PIP_STAMP "${CMAKE_BINARY_DIR}/.pip_requirements.stamp")
SET(_PIP_REQUIREMENTS "${CMAKE_CURRENT_SOURCE_DIR}/src/generator/requirements.txt")
IF(NOT EXISTS "${_PIP_STAMP}" OR "${_PIP_REQUIREMENTS}" IS_NEWER_THAN "${_PIP_STAMP}")
    IF(ENABLE_VENV)
        EXECUTE_PROCESS(COMMAND ${Python_EXECUTABLE} -m pip install --upgrade -r "${_PIP_REQUIREMENTS}")
    ELSE()
        EXECUTE_PROCESS(COMMAND ${Python_EXECUTABLE} -m pip install --user --upgrade -r "${_PIP_REQUIREMENTS}")
    ENDIF()
    FILE(TOUCH "${_PIP_STAMP}")
ENDIF()
UNSET(_PIP_STAMP)
UNSET(_PIP_REQUIREMENTS)
