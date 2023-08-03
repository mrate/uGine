project(materialBuilder LANGUAGES C CXX)

add_library(
    materialBuilder
    STATIC
        include/MaterialBuilder.h
        src/MaterialBuilder.cpp
)

target_include_directories(
    materialBuilder
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
    materialBuilder
    PUBLIC
        uGine::uGine
)

add_library(uGine::materialBuilder ALIAS materialBuilder)

add_executable(
    materialb
        src/materialb.cpp
)

target_link_libraries(
    materialb
        uGine::materialBuilder
)