project(sourceEmbed LANGUAGES C CXX)

add_library(
    sourceEmbedHeaders
    INTERFACE
        include/ugineTools/Embed.h
)

target_include_directories(
    sourceEmbedHeaders
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_library(sourceEmbed::Headers ALIAS sourceEmbedHeaders)

add_executable(
    sourceEmbed
        src/sourceEmbed.cpp
)
