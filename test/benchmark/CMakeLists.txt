INCLUDE(FetchContent)

SET(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
SET(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
FETCHCONTENT_DECLARE(
    google_benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.9.4
    GIT_SHALLOW TRUE
)
FETCHCONTENT_MAKEAVAILABLE(google_benchmark)

ADD_EXECUTABLE(datapoints_benchmark datapoints.cpp)

SET(_GEN_STAMP "${CMAKE_CURRENT_BINARY_DIR}/.datalayer_generated.stamp")
FILE(GLOB _BENCHMARK_MODEL_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../catch2/datapoints/model/*")
ADD_CUSTOM_COMMAND(
    OUTPUT "${_GEN_STAMP}"
    COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/../../src/generator/generator.py
            --model_dir ${CMAKE_CURRENT_SOURCE_DIR}/../catch2/datapoints/model
            --out_dir ${CMAKE_CURRENT_BINARY_DIR}
            --template_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../src/template
            --schema_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../src/generator
    COMMAND ${CMAKE_COMMAND} -E touch "${_GEN_STAMP}"
    DEPENDS ${_BENCHMARK_MODEL_FILES}
    COMMENT "DataLayerGenerator: generating benchmark datalayer"
    VERBATIM
)
ADD_CUSTOM_TARGET(datapointsBenchmarkGenerateDataLayer DEPENDS "${_GEN_STAMP}")

TARGET_INCLUDE_DIRECTORIES(datapoints_benchmark
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/generated/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src/include)
TARGET_LINK_LIBRARIES(datapoints_benchmark
    PRIVATE
        project_warnings
        project_options
        benchmark::benchmark_main)
ADD_DEPENDENCIES(datapoints_benchmark datapointsBenchmarkGenerateDataLayer)
