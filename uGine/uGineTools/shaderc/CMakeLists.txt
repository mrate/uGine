project(shaderc LANGUAGES C CXX)

add_library(
    shaderImporter
        src/ShaderDefinition.h
        src/ShaderDefinition.cpp
        src/ShaderImporter.cpp
        
        include/ShaderImporter.h
)

target_include_directories(
    shaderImporter
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
    shaderImporter
    PUBLIC
        uGine
)

add_executable(
    shaderc
        src/shaderc.cpp
)

target_link_libraries(
    shaderc
        shaderImporter
)

add_library(uGine::shaderImporter ALIAS shaderImporter)
