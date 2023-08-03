cmake_minimum_required(VERSION 3.24)

project(uGine VERSION 0.3.1)

configure_file(ugineConfig.h.in ugineConfig.h)

add_subdirectory(uGineTools)
add_subdirectory(uGineGfx)
add_subdirectory(uGineFoundation)
add_subdirectory(uGineEngine)
add_subdirectory(uGineRuntime)