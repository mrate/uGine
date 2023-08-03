cmake_minimum_required (VERSION 3.24)

macro(ugine_dependency name dir)
    message("Dependency ${name}:")

    list(APPEND CMAKE_MESSAGE_INDENT "    ")
    add_subdirectory(${dir})
    list(POP_BACK CMAKE_MESSAGE_INDENT)
endmacro()

#############################
#   Vulkan
#############################
list(APPEND CMAKE_MESSAGE_INDENT "    ")
find_package(Vulkan REQUIRED COMPONENTS OPTIONAL_COMPONENTS shaderc_combined)
list(POP_BACK CMAKE_MESSAGE_INDENT)

#############################
#   Assimp
#############################
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(BUILD_SHARED_LIBS OFF)
set(ASSIMP_NO_EXPORT ON)
set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_INSTALL OFF)

if (ANDROID)
    set(ASSIMP_ANDROID_JNIIOSYSTEM ON)
endif ()

ugine_dependency("Assimp" assimp)

#############################
#   EnkiTS
#############################
set(ENKITS_BUILD_C_INTERFACE    OFF)
set(ENKITS_BUILD_EXAMPLES       OFF)
set(ENKITS_BUILD_SHARED         OFF)
set(ENKITS_INSTALL              OFF)

ugine_dependency("EnkiTS" enkiTS)

add_library(enkiTS::enkiTS ALIAS enkiTS)

#############################
#   EnTT
#############################
ugine_dependency("EnTT" entt)

#############################
#   FileWatch
#############################
add_library(
    FileWatch
    INTERFACE
        filewatch/FileWatch.hpp
)

target_include_directories(
    FileWatch
    INTERFACE
        filewatch
)

#############################
#   GLM
#############################
ugine_dependency("GLM" glm)

#############################
#   Google test
#############################
ugine_dependency("GTest" googletest)

#############################
#   IconFontCppHeaders
#############################
add_library(
    IconFontCppHeaders
    INTERFACE
)

target_include_directories(
    IconFontCppHeaders
    INTERFACE
        IconFontCppHeaders
)

#############################
#   ImGui
#############################
message("[ugine] Dependency ImGui:")

add_library(
    ImGui
    STATIC
        imgui/imgui.cpp
        imgui/imgui.h
        imgui/imgui_demo.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_tables.cpp 
        imgui/imgui_widgets.cpp
        imgui/backends/imgui_impl_vulkan.h
        imgui/backends/imgui_impl_vulkan.cpp
)

if (WIN32)
    target_sources(
        ImGui
        PRIVATE
            imgui/backends/imgui_impl_win32.h
            imgui/backends/imgui_impl_win32.cpp
    )
endif()

target_include_directories(
    ImGui
    PUBLIC
        imgui
)

target_compile_definitions(
    ImGui
    PUBLIC
        IMGUI_DOCKING
        IMGUI_USE_WCHAR32
        IMGUI_DEFINE_MATH_OPERATORS
)

target_link_libraries(
    ImGui
    PRIVATE
        Vulkan::Vulkan
)

#############################
#   ImGuizmo
#############################
message("[ugine] Dependency ImGuizmo:")

add_library(
    ImGuizmo
    STATIC
        ImGuizmo/ImCurveEdit.cpp
        ImGuizmo/ImGradient.cpp 
        ImGuizmo/ImGuizmo.cpp 
        ImGuizmo/ImSequencer.cpp
)

target_include_directories(
    ImGuizmo
    PUBLIC
        ImGuizmo
)

target_compile_definitions(
    ImGuizmo
    PUBLIC
        VGIZMO_USES_GLM
)

target_link_libraries(
    ImGuizmo
    PUBLIC
        ImGui
)

#############################
#   nlohmann JSON
#############################
ugine_dependency("JSON" json)

#############################
#   KTX-Software
#############################
set(KTX_FEATURE_STATIC_LIBRARY ON)
set(KTX_FEATURE_TESTS OFF)
set(KTX_FEATURE_TOOLS OFF)

ugine_dependency("KTX" KTX-Software)

#############################
#   Mesh Optimizer
#############################
ugine_dependency("MeshOptimizer" meshoptimizer)

#############################
#   Mesh Optimizer
#############################
#set(OPTICK_USE_VULKAN ON)
#set(OPTICK_INSTALL_TARGETS OFF)
#
#ugine_dependency("Optick" optick)

#############################
#   SPDLOG
#############################
if (WIN32)
    set(SPDLOG_USE_STD_FORMAT ON)
endif ()

ugine_dependency("spdlog" spdlog)

#############################
#   SPIRV-Reflect
#############################
set(SPIRV_REFLECT_EXECUTABLE     OFF CACHE BOOL "opt")
set(SPIRV_REFLECT_EXAMPLES       OFF CACHE BOOL "opt")
set(SPIRV_REFLECT_STRIPPER       OFF CACHE BOOL "opt")
set(SPIRV_REFLECT_STATIC_LIB     ON CACHE BOOL "opt")
set(SPIRV_REFLECT_BUILD_TESTS    OFF CACHE BOOL "opt")
set(SPIRV_REFLECT_ENABLE_ASSERTS OFF CACHE BOOL "opt")

ugine_dependency("SPIRV-Reflect" SPIRV-Reflect)

#############################
#   STB Image
#############################
add_library(
    stb
    INTERFACE
        stb/stb_image.h
)

target_include_directories(
    stb
    INTERFACE
        stb
)

#############################
#   TLSF
#############################
add_library(
    tlsf
    STATIC
        tlsf/tlsf.c
        tlsf/tlsf.h
)

target_include_directories(
    tlsf
    PUBLIC
        tlsf
)

#############################
#   EnkiTS
#############################
ugine_dependency("Tracy" tracy)

##############################
##   Vulkan Memory Allocator HPP
##############################
ugine_dependency("VMA" VulkanMemoryAllocator-Hpp2)

##############################
##   bitsery
##############################
ugine_dependency("Bitsery" bitsery)

##############################
##   Lua
##############################
#set(LUABRIDGE_CXX17 1)
#ugine_dependency(lua)

#set(LUAJIT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/luajit)
ugine_dependency("LuaJit" LuaJitCMake)
target_include_directories(libluajit PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/LuaJitCMake/src)

ugine_dependency("LuaBridge3" LuaBridge3)

##############################
##   mimalloc
##############################
set(MI_BUILD_SHARED     OFF)
set(MI_BUILD_SHARED     OFF)
set(MI_BUILD_OBJECT     OFF)
set(MI_BUILD_TESTS      OFF)

ugine_dependency("mimalloc" mimalloc)

##############################
##   DXC
##############################
if (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/dxc)
    file(
        DOWNLOAD 
            https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.7.2212.1/dxc_2023_03_01.zip
            ${CMAKE_CURRENT_SOURCE_DIR}/dxc.zip
        SHOW_PROGRESS
    )

    file(
        ARCHIVE_EXTRACT
            INPUT ${CMAKE_CURRENT_SOURCE_DIR}/dxc.zip
            DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/dxc
            VERBOSE
    )
endif ()

add_executable(dxc_compiler IMPORTED GLOBAL)
set_target_properties(dxc_compiler PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/dxc/bin/x64/dxc.exe)

add_library(
    dxc_lib
    SHARED IMPORTED GLOBAL
)

set_target_properties(
    dxc_lib
    PROPERTIES 
        IMPORTED_IMPLIB  ${CMAKE_CURRENT_SOURCE_DIR}/dxc/lib/x64/dxcompiler.lib
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/dxc/bin/x64/dxcompiler.dll
)

target_include_directories(
    dxc_lib
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/dxc/inc
)

target_link_directories(
    dxc_lib
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/dxc/lib/x64
)
