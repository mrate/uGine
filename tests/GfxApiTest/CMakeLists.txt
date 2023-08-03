set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
set(SHADER_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../uGine/ugineEngine/ugine/engine/shaders)

embed_shader(src/shaders/triangle.hlsl ${GENERATED_DIR}/shaders triangle_vs_hlsl triangle_vs vs INCLUDE_DIR ${SHADER_INCLUDE_DIR} ENTRY_POINT vs)
embed_shader(src/shaders/triangle.hlsl ${GENERATED_DIR}/shaders triangle_fs_hlsl triangle_fs ps INCLUDE_DIR ${SHADER_INCLUDE_DIR} ENTRY_POINT ps)

embed_shader(src/shaders/bindless.hlsl ${GENERATED_DIR}/shaders bindless_vs_hlsl bindless_vs vs INCLUDE_DIR ${SHADER_INCLUDE_DIR} ENTRY_POINT vs)
embed_shader(src/shaders/bindless.hlsl ${GENERATED_DIR}/shaders bindless_fs_hlsl bindless_fs ps INCLUDE_DIR ${SHADER_INCLUDE_DIR} ENTRY_POINT ps)

add_executable(
	GfxApiTest
		src/main.cpp
		src/GfxTest.cpp
		src/GfxTest.h

		src/TestBindless.cpp
		src/TestBindless.h
		src/TestThreading.cpp
		src/TestThreading.h
		src/TestTriangle.cpp
		src/TestTriangle.h

		${GENERATED_DIR}/shaders/triangle_vs_hlsl.cpp
		${GENERATED_DIR}/shaders/triangle_vs_hlsl.h
		${GENERATED_DIR}/shaders/triangle_fs_hlsl.cpp
		${GENERATED_DIR}/shaders/triangle_fs_hlsl.h
		${GENERATED_DIR}/shaders/bindless_vs_hlsl.cpp
		${GENERATED_DIR}/shaders/bindless_vs_hlsl.h
		${GENERATED_DIR}/shaders/bindless_fs_hlsl.cpp
		${GENERATED_DIR}/shaders/bindless_fs_hlsl.h
)

target_include_directories(
	GfxApiTest
	PRIVATE
		${GENERATED_DIR}
)

target_link_libraries(
	GfxApiTest
		uGine
		sourceEmbed::Headers
)