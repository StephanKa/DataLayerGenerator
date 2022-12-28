SET(PROJECT_NAME DatalayerGeneratorExample)

ADD_EXECUTABLE(${PROJECT_NAME} main.cpp)
TARGET_LINK_LIBRARIES(
        ${PROJECT_NAME}
        PRIVATE project_options
        project_warnings
        CONAN_PKG::fmt)

ADD_CUSTOM_TARGET(GenerateDataLayer
                  COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generator/generator.py
                  --source_dir ${CMAKE_CURRENT_SOURCE_DIR}
                  --out_dir ${CMAKE_BINARY_DIR}
                  --template_dir ${CMAKE_CURRENT_SOURCE_DIR}/template
                  --schema_dir ${CMAKE_CURRENT_SOURCE_DIR}/generator)

TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/generated ${CMAKE_CURRENT_SOURCE_DIR}/include)

ADD_DEPENDENCIES(${PROJECT_NAME} GenerateDataLayer)
