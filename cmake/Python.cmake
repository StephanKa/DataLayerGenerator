FIND_PACKAGE(Python COMPONENTS Interpreter REQUIRED)

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

# install PyPI Python package using pip
EXECUTE_PROCESS(COMMAND ${Python_EXECUTABLE} -m pip install --upgrade pip -r ${CMAKE_CURRENT_SOURCE_DIR}/src/generator/requirements.txt)
