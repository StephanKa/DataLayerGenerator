# ProjectSettings.cmake
# Early project-level settings applied before the project() command.
# Sets CMake policies for CMake 3.25+ behavior.

# Ensure policies up to CMake 3.25 use NEW behavior
CMAKE_POLICY(SET CMP0135 NEW)  # URL download timestamp

# Prefer EXPORT_COMPILE_COMMANDS for modern tooling
SET(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "" FORCE)
