cmake_minimum_required(VERSION 3.24)

project(uGine)

option(UGINE_TRACE_ALLOCATIONS      CACHE   OFF)
option(UGINE_TRACE_ALLOCATIONS_CNT  CACHE   OFF)
option(UGINE_PROFILE                CACHE   ON)
option(UGINE_BUILD_TESTS            CACHE   ON)
option(UGINE_WARNINGS_AS_ERRORS     CACHE   ON)

message("[ugine] UGINE_TRACE_ALLOCATIONS     = ${UGINE_TRACE_ALLOCATIONS}")
message("[ugine] UGINE_TRACE_ALLOCATIONS_CNT = ${UGINE_TRACE_ALLOCATIONS_CNT}")
message("[ugine] UGINE_PROFILE               = ${UGINE_PROFILE}")
message("[ugine] UGINE_BUILD_TESTS           = ${UGINE_BUILD_TESTS}")
message("[ugine] UGINE_WARNINGS_AS_ERRORS    = ${UGINE_WARNINGS_AS_ERRORS}")

if (UGINE_BUILD_TESTS)
    enable_testing()
endif()

if (UGINE_PROFILE)
    set(TRACY_ENABLE ON)
    set(TRACY_FIBERS ON)
else ()
    set(TRACY_ENABLE OFF)
endif ()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(global_options)

#
# Compiler flags.
#

add_library(ugineCompilerFlags INTERFACE)
add_library(uGine::CompilerFlags ALIAS ugineCompilerFlags)

set(msvc_cxx "$<COMPILE_LANG_AND_ID:CXX,MSVC>")

target_compile_options(ugineCompilerFlags INTERFACE "$<${msvc_cxx}:-DUNICODE;-D_UNICODE;-DNOMINMAX>")
target_compile_options(ugineCompilerFlags INTERFACE "$<${msvc_cxx}:$<BUILD_INTERFACE:-W3>>")
target_compile_options(ugineCompilerFlags INTERFACE $<$<CXX_COMPILER_ID:MSVC>:/arch:AVX2>)

if (${UGINE_WARNINGS_AS_ERRORS})
    target_compile_options(ugineCompilerFlags INTERFACE "$<${msvc_cxx}:$<BUILD_INTERFACE:-WX>>")
endif ()

#
# Output directories
#

set(CMAKE_DEBUG_POSTFIX "d")

add_subdirectory(lib)

enable_testing()

add_subdirectory(uGine)

if (NOT ANDROID)
    add_subdirectory(editor)
    add_subdirectory(tests)
endif ()
