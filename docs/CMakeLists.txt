# docs/CMakeLists.txt
#
# Build dependencies (Python packages):
#   pip install sphinx sphinx-rtd-theme breathe sphinx-copybutton
#
# Targets produced:
#   Doxygen   — generates XML from src/include/ headers
#   Sphinx    — renders HTML documentation (depends on Doxygen)
#
# Neither target is in ALL by default. Build explicitly:
#   cmake --build <build> --target Sphinx

SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")

FIND_PACKAGE(Doxygen REQUIRED)
FIND_PACKAGE(Sphinx REQUIRED)

# ---------------------------------------------------------------------------
# Doxygen — generates XML consumed by Breathe / Sphinx
# ---------------------------------------------------------------------------
FILE(GLOB_RECURSE PROJECT_PUBLIC_HEADERS "${PROJECT_SOURCE_DIR}/src/include/*.h")

SET(DOXYGEN_INPUT_DIR  "${PROJECT_SOURCE_DIR}/src/include")
SET(DOXYGEN_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/doxygen")
SET(DOXYGEN_INDEX_FILE "${DOXYGEN_OUTPUT_DIR}/xml/index.xml")
SET(DOXYFILE_IN        "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in")
SET(DOXYFILE_OUT       "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile")

CONFIGURE_FILE(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)
FILE(MAKE_DIRECTORY ${DOXYGEN_OUTPUT_DIR})

ADD_CUSTOM_COMMAND(
    OUTPUT  "${DOXYGEN_INDEX_FILE}"
    DEPENDS ${PROJECT_PUBLIC_HEADERS}
    COMMAND "${DOXYGEN_EXECUTABLE}" "${DOXYFILE_OUT}"
    MAIN_DEPENDENCY "${DOXYFILE_OUT}" "${DOXYFILE_IN}"
    COMMENT "Generating Doxygen XML from src/include/"
    VERBATIM
)

# Not in ALL — only built when Sphinx target is requested
ADD_CUSTOM_TARGET(Doxygen DEPENDS "${DOXYGEN_INDEX_FILE}")

# ---------------------------------------------------------------------------
# Sphinx — renders RST + Breathe (Doxygen XML) → HTML
# ---------------------------------------------------------------------------
SET(SPHINX_SOURCE     "${CMAKE_CURRENT_SOURCE_DIR}")
SET(SPHINX_BUILD      "${CMAKE_CURRENT_BINARY_DIR}/sphinx")
SET(SPHINX_INDEX_FILE "${SPHINX_BUILD}/index.html")

# Collect all RST source files so Sphinx is re-triggered on any change
FILE(GLOB_RECURSE SPHINX_RST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.rst")

ADD_CUSTOM_COMMAND(
    OUTPUT  "${SPHINX_INDEX_FILE}"
    COMMAND "${SPHINX_EXECUTABLE}"
            -b html
            -Dbreathe_projects.DataLayerGenerator="${DOXYGEN_OUTPUT_DIR}/xml"
            -Dhtml_theme=sphinx_rtd_theme
            "${SPHINX_SOURCE}"
            "${SPHINX_BUILD}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    DEPENDS
        ${SPHINX_RST_FILES}
        "${CMAKE_CURRENT_SOURCE_DIR}/conf.py"
        "${DOXYGEN_INDEX_FILE}"
    COMMENT "Generating HTML documentation with Sphinx"
    VERBATIM
)

# Not in ALL — build explicitly with: cmake --build <dir> --target Sphinx
ADD_CUSTOM_TARGET(Sphinx DEPENDS "${SPHINX_INDEX_FILE}")

# Make Sphinx depend on Doxygen so the two targets can be built independently
ADD_DEPENDENCIES(Sphinx Doxygen)

# ---------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------
INCLUDE(GNUInstallDirs)
INSTALL(DIRECTORY "${SPHINX_BUILD}"
        DESTINATION "${CMAKE_INSTALL_DOCDIR}"
        OPTIONAL)
