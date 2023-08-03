project(materialc LANGUAGES C CXX)

add_library(
    materialImporter
    STATIC
        include/MaterialImporter.h
        src/MaterialImporter.cpp

        ${CMAKE_CURRENT_SOURCE_DIR}/../../ugineEngine/ugine/engine/gfx/asset/SerializedMaterial.cpp
)

target_include_directories(
    materialImporter
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(
    materialImporter
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../../ugineEngine
        ${CMAKE_BINARY_DIR}
)

target_link_libraries(
    materialImporter
    PUBLIC
        glm::glm
        EnTT::EnTT
        enkiTS::enkiTS
        spdlog::spdlog
        nlohmann_json
        Bitsery::bitsery
        
        uGine::Gfx
)

add_library(uGine::materialImporter ALIAS materialImporter)

add_executable(
    materialc
        src/materialc.cpp
)

target_include_directories(
    materialc
    PUBLIC
        ${CMAKE_BINARY_DIR}
)

target_link_libraries(
    materialc
        uGine::materialImporter
)

add_custom_command(
  TARGET materialc POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:materialc> $<TARGET_FILE_DIR:materialc>
  COMMAND_EXPAND_LISTS
)
