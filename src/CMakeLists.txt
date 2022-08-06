SET(PROJECT_NAME DatalayerGeneratorExample)

ADD_EXECUTABLE(${PROJECT_NAME} main.cpp)
TARGET_LINK_LIBRARIES(
        ${PROJECT_NAME}
        PRIVATE project_options
        project_warnings
        CONAN_PKG::fmt)

ADD_CUSTOM_TARGET(GenerateDataLayer
                  COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/generator/generator.py -s ${CMAKE_CURRENT_SOURCE_DIR} -o ${CMAKE_BINARY_DIR})

TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/generated ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_BINARY_DIR}/generated)

ADD_DEPENDENCIES(${PROJECT_NAME} GenerateDataLayer)
