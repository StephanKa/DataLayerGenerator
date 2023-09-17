SET(PROJECT_NAME DatalayerGeneratorExample)

IF(CMAKE_CROSSCOMPILING)
    ADD_EXECUTABLE(${PROJECT_NAME} embedded.cpp)
    TARGET_LINK_LIBRARIES(
            ${PROJECT_NAME}
            PRIVATE project_options
            project_warnings)

    SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES SUFFIX ".elf")
    TARGET_LINK_OPTIONS(${PROJECT_NAME}
                        PRIVATE
                        -T${CMAKE_CURRENT_SOURCE_DIR}/linkerscript/linkerScript.ld
                        -Wl,-Map=${PROJECT_NAME}.map
    )

    ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD
                       COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/generated ${CMAKE_BINARY_DIR}/install/generated)

    ADD_CUSTOM_TARGET(${PROJECT_NAME}.bin ALL
                      DEPENDS ${PROJECT_NAME}
                      COMMAND ${CMAKE_OBJCOPY} -Obinary ${CMAKE_BINARY_DIR}/src/${PROJECT_NAME}.elf ${CMAKE_BINARY_DIR}/install/${PROJECT_NAME}.bin
                      COMMAND ${CMAKE_OBJCOPY} -Oihex ${CMAKE_BINARY_DIR}/src/${PROJECT_NAME}.elf ${CMAKE_BINARY_DIR}/install/${PROJECT_NAME}.hex
    )

ELSE()
    FIND_PACKAGE(fmt REQUIRED)

    ADD_EXECUTABLE(${PROJECT_NAME} main.cpp)
    TARGET_LINK_LIBRARIES(
            ${PROJECT_NAME}
            PRIVATE project_options
            project_warnings
            fmt::fmt-header-only)

    ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD
                       COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ${CMAKE_BINARY_DIR}/install/bin/$<TARGET_FILE_NAME:${PROJECT_NAME}>)

    ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD
                       COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/generated ${CMAKE_BINARY_DIR}/install/generated)
    IF(ENABLE_PYBIND11)
        LIST(APPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}")
        FIND_PACKAGE(Python COMPONENTS Interpreter Development REQUIRED)
        FIND_PACKAGE(pybind11 REQUIRED)

        SET_SOURCE_FILES_PROPERTIES(${CMAKE_BINARY_DIR}/generated/python/pythonBinding.cpp PROPERTIES GENERATED TRUE)
        PYBIND11_ADD_MODULE(conan_pybind11 ${CMAKE_BINARY_DIR}/generated/python/pythonBinding.cpp)
        TARGET_LINK_LIBRARIES(conan_pybind11
                              PRIVATE
                              project_warnings
                              fmt::fmt-header-only)
        TARGET_INCLUDE_DIRECTORIES(conan_pybind11 PRIVATE ${CMAKE_BINARY_DIR}/generated ${CMAKE_CURRENT_SOURCE_DIR}/include)
        ADD_DEPENDENCIES(conan_pybind11 GenerateDataLayer)
        ADD_CUSTOM_COMMAND(TARGET conan_pybind11 POST_BUILD
                           COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:conan_pybind11> ${CMAKE_BINARY_DIR}/install/bin/$<TARGET_FILE_NAME:conan_pybind11>)
        ADD_CUSTOM_COMMAND(TARGET conan_pybind11 POST_BUILD
                           COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/generated ${CMAKE_BINARY_DIR}/install/generated)
    ENDIF()
ENDIF()


ADD_CUSTOM_TARGET(GenerateDataLayer
                  COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generator/generator.py
                  --model_dir ${CMAKE_CURRENT_SOURCE_DIR}/model
                  --out_dir ${CMAKE_BINARY_DIR}
                  --template_dir ${CMAKE_CURRENT_SOURCE_DIR}/template
                  --schema_dir ${CMAKE_CURRENT_SOURCE_DIR}/generator)

TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/generated ${CMAKE_CURRENT_SOURCE_DIR}/include)
ADD_DEPENDENCIES(${PROJECT_NAME} GenerateDataLayer)
