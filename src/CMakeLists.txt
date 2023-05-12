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
    ADD_CUSTOM_TARGET(${PROJECT_NAME}.bin ALL
                      DEPENDS ${PROJECT_NAME}
                      COMMAND ${CMAKE_OBJCOPY} -Obinary ${CMAKE_BINARY_DIR}/bin/${PROJECT_NAME}.elf ${CMAKE_BINARY_DIR}/bin/${PROJECT_NAME}.bin
                      COMMAND ${CMAKE_OBJCOPY} -Oihex ${CMAKE_BINARY_DIR}/bin/${PROJECT_NAME}.elf ${CMAKE_BINARY_DIR}/bin/${PROJECT_NAME}.hex
                      )
ELSE()
    ADD_EXECUTABLE(${PROJECT_NAME} main.cpp)
    TARGET_LINK_LIBRARIES(
            ${PROJECT_NAME}
            PRIVATE project_options
            project_warnings
            CONAN_PKG::fmt)
ENDIF()


ADD_CUSTOM_TARGET(GenerateDataLayer
        COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generator/generator.py
        --model_dir ${CMAKE_CURRENT_SOURCE_DIR}/model
        --out_dir ${CMAKE_BINARY_DIR}
        --template_dir ${CMAKE_CURRENT_SOURCE_DIR}/template
        --schema_dir ${CMAKE_CURRENT_SOURCE_DIR}/generator)

TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/generated ${CMAKE_CURRENT_SOURCE_DIR}/include)
ADD_DEPENDENCIES(${PROJECT_NAME} GenerateDataLayer)
