find_package(Vulkan REQUIRED COMPONENTS OPTIONAL_COMPONENTS shaderc_combined glslc)

option(UGINE_VK_PUSH_DESCRIPTORS CACHE ON)
option(UGINE_VK_VALIDATION CACHE ON)
option(UGINE_VK_TRACE_ALLOCATIONS CACHE OFF)
option(UGINE_VK_TRACE_SYNC CACHE OFF)

message("[ugine] UGINE_VK_PUSH_DESCRIPTORS = ${UGINE_VK_PUSH_DESCRIPTORS}")
message("[ugine] UGINE_VK_VALIDATION = ${UGINE_VK_VALIDATION}")
message("[ugine] UGINE_VK_TRACE_ALLOCATIONS = ${UGINE_VK_TRACE_ALLOCATIONS}")
message("[ugine] UGINE_VK_TRACE_SYNC = ${UGINE_VK_TRACE_SYNC}")

function(embed_shader SRC_FILE OUTPUT_DIRECTORY OUTPUT_FILE VAR_NAME SHADER_STAGE)
	set(flags)
	set(args ENTRY_POINT INCLUDE_DIR)
	set(listArgs)
	cmake_parse_arguments(arg "${flags}" "${args}" "${listArgs}" ${ARGN})

	if (NOT arg_ENTRY_POINT)
		set(arg_ENTRY_POINT main)
	endif()

	set(INCLUDE_ARG)
	if (arg_INCLUDE_DIR)
		set(INCLUDE_ARG "-I ${arg_INCLUDE_DIR}")
	endif()

	set(BIN_FILE ${OUTPUT_DIRECTORY}/${OUTPUT_FILE}.spv)

	if (CMAKE_BUILD_TYPE MATCHES Debug)
		set(COMPILE_ARGS "-fspv-debug=vulkan-with-source" "-Od")
	else()
		set(COMPILE_ARGS "-O2")
	endif()

	add_custom_command( 
		COMMAND dxc_compiler ${SRC_FILE} -Fo ${BIN_FILE} -spirv -E ${arg_ENTRY_POINT} ${COMPILE_ARGS} -T ${SHADER_STAGE}_6_0 ${INCLUDE_ARG}
		COMMAND ${CMAKE_COMMAND} -E echo "Embedding HLSL shader '${SRC_FILE}'" # Ninja's don't respect COMMENT
		OUTPUT ${BIN_FILE}
		DEPENDS ${SRC_FILE}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		VERBATIM
	)

	add_custom_command(
		COMMAND sourceEmbed ${BIN_FILE} ${OUTPUT_DIRECTORY} ${OUTPUT_FILE}.h ${VAR_NAME}
		OUTPUT ${OUTPUT_DIRECTORY}/${OUTPUT_FILE}.h ${OUTPUT_DIRECTORY}/${OUTPUT_FILE}.cpp
		DEPENDS ${BIN_FILE} sourceEmbed
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		VERBATIM
	)
endfunction()

function(embed_file SRC_FILE OUTPUT_DIRECTORY OUTPUT_FILE VAR_NAME)
	add_custom_command(
		COMMAND sourceEmbed ${SRC_FILE} ${OUTPUT_DIRECTORY} ${OUTPUT_FILE}.h ${VAR_NAME}
		OUTPUT ${OUTPUT_DIRECTORY}/${OUTPUT_FILE}.h ${OUTPUT_DIRECTORY}/${OUTPUT_FILE}.cpp
		DEPENDS ${BIN_FILE} sourceEmbed
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		VERBATIM
	)
endfunction()

function(embed_shader_resource SRC_FILE OUTPUT_DIRECTORY OUTPUT_FILE VAR_NAME INCLUDE_DIR)
	set(BIN_FILE ${OUTPUT_DIRECTORY}/${OUTPUT_FILE}.ush)

	# Add -S for debugging (saves each variant stage).
	add_custom_command( 
		COMMAND shaderc ${SRC_FILE} ${BIN_FILE} -I ${INCLUDE_DIR} -W -G
		COMMAND ${CMAKE_COMMAND} -E echo "Embedding shader '${SRC_FILE}'" # Ninja's don't respect COMMENT
		OUTPUT ${BIN_FILE}
		DEPENDS shaderc ${SRC_FILE}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		VERBATIM
	)

	add_custom_command(
		COMMAND sourceEmbed ${BIN_FILE} ${OUTPUT_DIRECTORY} ${OUTPUT_FILE}.h ${VAR_NAME}
		OUTPUT ${OUTPUT_DIRECTORY}/${OUTPUT_FILE}.h ${OUTPUT_DIRECTORY}/${OUTPUT_FILE}.cpp
		DEPENDS ${BIN_FILE} sourceEmbed
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		VERBATIM
	)
endfunction()

function(embed_material SRC_FILE OUTPUT_DIRECTORY OUTPUT_FILE VAR_NAME INCLUDE_DIR)
	set(BIN_FILE ${OUTPUT_DIRECTORY}/${OUTPUT_FILE}.umat)

	add_custom_command( 
		COMMAND materialc ${SRC_FILE} ${BIN_FILE} ${INCLUDE_DIR}
		COMMAND ${CMAKE_COMMAND} -E echo "Embedding material '${SRC_FILE}'" # Ninja's don't respect COMMENT
		OUTPUT ${BIN_FILE}
		DEPENDS materialc ${SRC_FILE}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		VERBATIM
	)

	add_custom_command(
		COMMAND sourceEmbed ${BIN_FILE} ${OUTPUT_DIRECTORY} ${OUTPUT_FILE}.h ${VAR_NAME}
		OUTPUT ${OUTPUT_DIRECTORY}/${OUTPUT_FILE}.h ${OUTPUT_DIRECTORY}/${OUTPUT_FILE}.cpp
		DEPENDS ${BIN_FILE} sourceEmbed
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		VERBATIM
	)
endfunction()

configure_file(ugineGfxConfig.h.in ugineGfxConfig.h)

add_library(
	uGineGfx
	STATIC
		gfxapi/BumpAllocator.h
		gfxapi/CommandList.h
		gfxapi/Device.h		
		gfxapi/Error.h
		gfxapi/FramebufferCache.cpp
		gfxapi/FramebufferCache.h
		gfxapi/Handle.cpp
		gfxapi/Handle.h
		gfxapi/Hash.cpp
		gfxapi/Hash.h
		gfxapi/Json.h
		gfxapi/QueryPool.h
		gfxapi/RenderTargetCache.cpp
		gfxapi/RenderTargetCache.h
		gfxapi/Swapchain.h
		gfxapi/Types.h
		gfxapi/WaitableEvent.h

		gfxapi/spirv/SpirvParser.cpp
		gfxapi/spirv/SpirvParser.h
		gfxapi/spirv/SpirvCompiler.h
		
		# HLSL [new]
		gfxapi/spirv/SpirvHlslCompiler.cpp
		gfxapi/spirv/SpirvHlslCompiler.h

		# GLSL [old]
		# gfxapi/spirv/SpirvGlslCompiler.cpp
		# gfxapi/spirv/SpirvGlslCompiler.h
		# gfxapi/spirv/SpirvGlslIncludeResolver.cpp
		# gfxapi/spirv/SpirvGlslIncludeResolver.h

		gfxapi/vulkan/Json.h
		gfxapi/vulkan/VmaWrapper.cpp
		gfxapi/vulkan/Vulkan.cpp
		gfxapi/vulkan/Vulkan.h
		gfxapi/vulkan/VulkanBumpAllocator.cpp
		gfxapi/vulkan/VulkanBumpAllocator.h
		gfxapi/vulkan/VulkanCommandList.cpp
		gfxapi/vulkan/VulkanCommandList.h
		gfxapi/vulkan/VulkanDescriptors.cpp
		gfxapi/vulkan/VulkanDescriptors.h
		gfxapi/vulkan/VulkanDescriptorSetPool.cpp
		gfxapi/vulkan/VulkanDescriptorSetPool.h
		gfxapi/vulkan/VulkanDescriptorSetLayoutCache.cpp
		gfxapi/vulkan/VulkanDescriptorSetLayoutCache.h
		gfxapi/vulkan/VulkanDevice.cpp
		gfxapi/vulkan/VulkanDevice.h
		
		gfxapi/vulkan/VulkanInitializers.cpp
		gfxapi/vulkan/VulkanInitializers.h
		gfxapi/vulkan/VulkanInstance.cpp
		gfxapi/vulkan/VulkanInstance.h
		gfxapi/vulkan/VulkanResourceList.h
		gfxapi/vulkan/VulkanResources.h
		gfxapi/vulkan/VulkanSwapchain.cpp
		gfxapi/vulkan/VulkanSwapchain.h
		gfxapi/vulkan/VulkanBindlessPool.cpp
		gfxapi/vulkan/VulkanBindlessPool.h
		gfxapi/vulkan/VulkanQueryPool.cpp
		gfxapi/vulkan/VulkanQueryPool.h
)

add_library(uGine::Gfx ALIAS uGineGfx)

target_include_directories(
	uGineGfx
	PUBLIC
		./
		${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(
	uGineGfx
	PUBLIC
		uGine::CompilerFlags
		uGine::Foundation

		ImGui
		glm::glm
		spirv-reflect-static
		Vulkan::Headers
		VulkanMemoryAllocator-Hpp
		nlohmann_json
)

if (NOT ANDROID)
	target_link_libraries(
		uGineGfx
		PRIVATE
			dxc_lib
			Vulkan::shaderc_combined
	)
endif()

target_compile_definitions(
	uGineGfx
	PRIVATE
		VK_USE_PLATFORM_WIN32_KHR
		VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
)

target_compile_definitions(
	uGineGfx
	PUBLIC
		GLM_FORCE_DEPTH_ZERO_TO_ONE
)