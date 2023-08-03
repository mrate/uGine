##########
#  uGine
##########
set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
set(SHADER_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ugine/engine/shaders)

# HLSL test
embed_shader(ugine/engine/shaders/embed/fullscreen.vert.hlsl ${GENERATED_DIR}/shaders fullscreen_vs_hlsl fullscreen_vs vs INCLUDE_DIR ${SHADER_INCLUDE_DIR})
embed_shader(ugine/engine/shaders/embed/fullscreen.frag.hlsl ${GENERATED_DIR}/shaders fullscreen_fs_hlsl fullscreen_fs ps INCLUDE_DIR ${SHADER_INCLUDE_DIR})
embed_shader(ugine/engine/shaders/embed/ui.vert.hlsl ${GENERATED_DIR}/shaders ui_vs_hlsl ui_vs vs INCLUDE_DIR ${SHADER_INCLUDE_DIR})
embed_shader(ugine/engine/shaders/embed/ui.frag.hlsl ${GENERATED_DIR}/shaders ui_fs_hlsl ui_fs ps INCLUDE_DIR ${SHADER_INCLUDE_DIR})

embed_shader(ugine/engine/shaders/embed/ssao.frag.hlsl ${GENERATED_DIR}/shaders ssao_fs_hlsl ssao_fs ps INCLUDE_DIR ${SHADER_INCLUDE_DIR})
embed_shader(ugine/engine/shaders/embed/tonemapping.frag.hlsl ${GENERATED_DIR}/shaders tonemapping_fs_hlsl tonemapping_fs ps INCLUDE_DIR ${SHADER_INCLUDE_DIR})

embed_shader(ugine/engine/shaders/embed/animation.comp.hlsl ${GENERATED_DIR}/shaders animation_cs_hlsl animation_cs cs INCLUDE_DIR ${SHADER_INCLUDE_DIR})
embed_shader(ugine/engine/shaders/embed/blur.comp.hlsl ${GENERATED_DIR}/shaders blur_cs_hlsl blur_cs cs INCLUDE_DIR ${SHADER_INCLUDE_DIR})

embed_shader(ugine/engine/shaders/embed/gizmo.frag.hlsl ${GENERATED_DIR}/shaders gizmo_fs_hlsl gizmo_fs ps INCLUDE_DIR ${SHADER_INCLUDE_DIR})
embed_shader(ugine/engine/shaders/embed/gizmoCircle.vert.hlsl ${GENERATED_DIR}/shaders gizmoCircle_vs_hlsl gizmoCircle_vs vs INCLUDE_DIR ${SHADER_INCLUDE_DIR})
embed_shader(ugine/engine/shaders/embed/gizmoCube.vert.hlsl ${GENERATED_DIR}/shaders gizmoCube_vs_hlsl gizmoCube_vs vs INCLUDE_DIR ${SHADER_INCLUDE_DIR})
embed_shader(ugine/engine/shaders/embed/gizmoIcosphere.vert.hlsl ${GENERATED_DIR}/shaders gizmoIcosphere_vs_hlsl gizmoIcosphere_vs vs INCLUDE_DIR ${SHADER_INCLUDE_DIR})
embed_shader(ugine/engine/shaders/embed/gizmoCone.vert.hlsl ${GENERATED_DIR}/shaders gizmoCone_vs_hlsl gizmoCone_vs vs INCLUDE_DIR ${SHADER_INCLUDE_DIR})
embed_shader(ugine/engine/shaders/embed/gizmoCylinder.vert.hlsl ${GENERATED_DIR}/shaders gizmoCylinder_vs_hlsl gizmoCylinder_vs vs INCLUDE_DIR ${SHADER_INCLUDE_DIR})
embed_shader(ugine/engine/shaders/embed/gizmoCapsule.vert.hlsl ${GENERATED_DIR}/shaders gizmoCapsule_vs_hlsl gizmoCapsule_vs vs INCLUDE_DIR ${SHADER_INCLUDE_DIR})
embed_shader(ugine/engine/shaders/embed/gizmoDir.vert.hlsl ${GENERATED_DIR}/shaders gizmoDir_vs_hlsl gizmoDir_vs vs INCLUDE_DIR ${SHADER_INCLUDE_DIR})
embed_shader(ugine/engine/shaders/embed/gizmoFrustum.vert.hlsl ${GENERATED_DIR}/shaders gizmoFrustum_vs_hlsl gizmoFrustum_vs vs INCLUDE_DIR ${SHADER_INCLUDE_DIR})
embed_shader(ugine/engine/shaders/embed/gizmoLine.vert.hlsl ${GENERATED_DIR}/shaders gizmoLine_vs_hlsl gizmoLine_vs vs INCLUDE_DIR ${SHADER_INCLUDE_DIR})
embed_shader(ugine/engine/shaders/embed/gizmoTexture.vert.hlsl ${GENERATED_DIR}/shaders gizmoTexture_vs_hlsl gizmoTexture_vs vs INCLUDE_DIR ${SHADER_INCLUDE_DIR})

embed_shader(ugine/engine/shaders/embed/debug.vert.hlsl ${GENERATED_DIR}/shaders debug_vs_hlsl debug_vs vs INCLUDE_DIR ${SHADER_INCLUDE_DIR})
embed_shader(ugine/engine/shaders/embed/debug.frag.hlsl ${GENERATED_DIR}/shaders debug_fs_hlsl debug_fs ps INCLUDE_DIR ${SHADER_INCLUDE_DIR})

embed_shader(ugine/engine/shaders/embed/outline.frag.hlsl ${GENERATED_DIR}/shaders outline_fs_hlsl outline_fs ps INCLUDE_DIR ${SHADER_INCLUDE_DIR})

embed_shader(ugine/engine/shaders/embed/lightCullGenFrustums.comp.hlsl ${GENERATED_DIR}/shaders lightCullGenFrustums_cs_hlsl lightCullGenFrustums_cs cs INCLUDE_DIR ${SHADER_INCLUDE_DIR})
embed_shader(ugine/engine/shaders/embed/lightCull.comp.hlsl ${GENERATED_DIR}/shaders lightCull_cs_hlsl lightCull_cs cs INCLUDE_DIR ${SHADER_INCLUDE_DIR})
embed_shader(ugine/engine/shaders/embed/lightCullDebug.frag.hlsl ${GENERATED_DIR}/shaders lightCullDebug_fs_hlsl lightCullDebug_fs ps INCLUDE_DIR ${SHADER_INCLUDE_DIR})

embed_file(ugine/engine/assets/fa-solid-900.ttf ${GENERATED_DIR}/fonts fa_font fa_font)
embed_file(ugine/engine/assets/Roboto-Medium.ttf ${GENERATED_DIR}/fonts roboto_font roboto_font)
embed_file(ugine/engine/assets/materialdesignicons-webfont.ttf ${GENERATED_DIR}/fonts md_font md_font)

add_library(
	uGine
	STATIC
		${GENERATED_DIR}/shaders/fullscreen_vs_hlsl.h
		${GENERATED_DIR}/shaders/fullscreen_vs_hlsl.cpp
		${GENERATED_DIR}/shaders/fullscreen_fs_hlsl.h
		${GENERATED_DIR}/shaders/fullscreen_fs_hlsl.cpp
		${GENERATED_DIR}/shaders/animation_cs_hlsl.h
		${GENERATED_DIR}/shaders/animation_cs_hlsl.cpp
		${GENERATED_DIR}/shaders/ui_vs_hlsl.h
		${GENERATED_DIR}/shaders/ui_vs_hlsl.cpp
		${GENERATED_DIR}/shaders/ui_fs_hlsl.h
		${GENERATED_DIR}/shaders/ui_fs_hlsl.cpp

		${GENERATED_DIR}/shaders/gizmo_fs_hlsl.h
		${GENERATED_DIR}/shaders/gizmo_fs_hlsl.cpp
		${GENERATED_DIR}/shaders/gizmoCircle_vs_hlsl.h
		${GENERATED_DIR}/shaders/gizmoCircle_vs_hlsl.cpp
		${GENERATED_DIR}/shaders/gizmoCube_vs_hlsl.h
		${GENERATED_DIR}/shaders/gizmoCube_vs_hlsl.cpp
		${GENERATED_DIR}/shaders/gizmoIcosphere_vs_hlsl.h
		${GENERATED_DIR}/shaders/gizmoIcosphere_vs_hlsl.cpp
		${GENERATED_DIR}/shaders/gizmoCone_vs_hlsl.h
		${GENERATED_DIR}/shaders/gizmoCone_vs_hlsl.cpp
		${GENERATED_DIR}/shaders/gizmoCylinder_vs_hlsl.h
		${GENERATED_DIR}/shaders/gizmoCylinder_vs_hlsl.cpp
		${GENERATED_DIR}/shaders/gizmoCapsule_vs_hlsl.h
		${GENERATED_DIR}/shaders/gizmoCapsule_vs_hlsl.cpp
		${GENERATED_DIR}/shaders/gizmoDir_vs_hlsl.h
		${GENERATED_DIR}/shaders/gizmoDir_vs_hlsl.cpp
		${GENERATED_DIR}/shaders/gizmoFrustum_vs_hlsl.h
		${GENERATED_DIR}/shaders/gizmoFrustum_vs_hlsl.cpp
		${GENERATED_DIR}/shaders/gizmoLine_vs_hlsl.h
		${GENERATED_DIR}/shaders/gizmoLine_vs_hlsl.cpp
		${GENERATED_DIR}/shaders/gizmoTexture_vs_hlsl.h
		${GENERATED_DIR}/shaders/gizmoTexture_vs_hlsl.cpp

		${GENERATED_DIR}/shaders/debug_vs_hlsl.h
		${GENERATED_DIR}/shaders/debug_vs_hlsl.cpp
		${GENERATED_DIR}/shaders/debug_fs_hlsl.h
		${GENERATED_DIR}/shaders/debug_fs_hlsl.cpp
		
		${GENERATED_DIR}/shaders/outline_fs_hlsl.h
		${GENERATED_DIR}/shaders/outline_fs_hlsl.cpp
		
		${GENERATED_DIR}/shaders/ssao_fs_hlsl.h
		${GENERATED_DIR}/shaders/ssao_fs_hlsl.cpp
		${GENERATED_DIR}/shaders/blur_cs_hlsl.h
		${GENERATED_DIR}/shaders/blur_cs_hlsl.cpp
		${GENERATED_DIR}/shaders/tonemapping_fs_hlsl.h
		${GENERATED_DIR}/shaders/tonemapping_fs_hlsl.cpp
		
		${GENERATED_DIR}/shaders/lightCullGenFrustums_cs_hlsl.h
		${GENERATED_DIR}/shaders/lightCullGenFrustums_cs_hlsl.cpp
		${GENERATED_DIR}/shaders/lightCull_cs_hlsl.h
		${GENERATED_DIR}/shaders/lightCull_cs_hlsl.cpp
		${GENERATED_DIR}/shaders/lightCullDebug_fs_hlsl.h
		${GENERATED_DIR}/shaders/lightCullDebug_fs_hlsl.cpp
		
		${GENERATED_DIR}/fonts/fa_font.h
		${GENERATED_DIR}/fonts/fa_font.cpp
		${GENERATED_DIR}/fonts/roboto_font.h
		${GENERATED_DIR}/fonts/roboto_font.cpp
		${GENERATED_DIR}/fonts/md_font.h
		${GENERATED_DIR}/fonts/md_font.cpp
				
		ugine/engine/core/CoreSystem.cpp
		ugine/engine/core/CoreSystem.h		
		ugine/engine/core/Json.h
		ugine/engine/core/Resource.h
		ugine/engine/core/Resource.cpp
		ugine/engine/core/ResourceID.h
		ugine/engine/core/ResourceManager.h

		
		ugine/engine/world/Component.h
		ugine/engine/world/Camera.cpp
		ugine/engine/world/Camera.h
		ugine/engine/world/GameObject.cpp
		ugine/engine/world/GameObject.h
		ugine/engine/world/World.cpp
		ugine/engine/world/World.h
		ugine/engine/world/Transformation.cpp
		ugine/engine/world/Transformation.h			
		ugine/engine/world/WorldManager.cpp
		ugine/engine/world/WorldManager.h
		ugine/engine/world/WorldSerializer.cpp
		ugine/engine/world/WorldSerializer.h

		ugine/engine/engine/Engine.cpp
		ugine/engine/engine/Engine.h
		ugine/engine/engine/Params.h
		ugine/engine/engine/System.h
		ugine/engine/engine/CVars.cpp
		ugine/engine/engine/CVars.h
		
		ugine/engine/gfx/asset/SerializedAnimation.cpp
		ugine/engine/gfx/asset/SerializedAnimation.h
		ugine/engine/gfx/asset/SerializedMaterial.cpp
		ugine/engine/gfx/asset/SerializedMaterial.h
		ugine/engine/gfx/asset/SerializedModel.cpp
		ugine/engine/gfx/asset/SerializedModel.h
		ugine/engine/gfx/asset/SerializedShader.cpp
		ugine/engine/gfx/asset/SerializedShader.h
		ugine/engine/gfx/Animation.cpp
		ugine/engine/gfx/Animation.h
		ugine/engine/gfx/Consts.h		
		ugine/engine/gfx/Component.h
		ugine/engine/gfx/GpuQuery.h
		ugine/engine/gfx/GraphicsScene.cpp
		ugine/engine/gfx/GraphicsScene.h
		ugine/engine/gfx/GraphicsState.cpp
		ugine/engine/gfx/GraphicsState.h
		ugine/engine/gfx/GraphicsSystem.cpp
		ugine/engine/gfx/GraphicsSystem.h
		ugine/engine/gfx/ImGui.cpp
		ugine/engine/gfx/ImGui.h
		ugine/engine/gfx/ImGuiSystem.cpp
		ugine/engine/gfx/ImGuiSystem.h
		ugine/engine/gfx/Material.cpp
		ugine/engine/gfx/Material.h
		ugine/engine/gfx/Model.cpp
		ugine/engine/gfx/Model.h
		ugine/engine/gfx/RenderContext.h
		ugine/engine/gfx/Pipeline.cpp
		ugine/engine/gfx/Pipeline.h
		ugine/engine/gfx/RenderThread.cpp
		ugine/engine/gfx/RenderThread.h
		ugine/engine/gfx/Shader.cpp
		ugine/engine/gfx/Shader.h
		ugine/engine/gfx/Shapes.cpp
		ugine/engine/gfx/Shapes.h
		ugine/engine/gfx/Texture.cpp
		ugine/engine/gfx/Texture.h
		ugine/engine/gfx/Uniform.h

		ugine/engine/gfx/pass/DepthPrePass.cpp
		ugine/engine/gfx/pass/DepthPrePass.h
		ugine/engine/gfx/pass/ForwardPass.cpp
		ugine/engine/gfx/pass/ForwardPass.h
		ugine/engine/gfx/pass/GeometryPass.cpp
		ugine/engine/gfx/pass/GeometryPass.h
		ugine/engine/gfx/pass/LightCullingPass.cpp
		ugine/engine/gfx/pass/LightCullingPass.h
		ugine/engine/gfx/pass/SsaoPass.cpp
		ugine/engine/gfx/pass/SsaoPass.h
		ugine/engine/gfx/pass/ShadowPass.cpp
		ugine/engine/gfx/pass/ShadowPass.h
		ugine/engine/gfx/pass/TonemappingPass.cpp
		ugine/engine/gfx/pass/TonemappingPass.h

		ugine/engine/gfx/helpers/DebugRenderer.cpp
		ugine/engine/gfx/helpers/DebugRenderer.h

		ugine/engine/input/Input.h
		ugine/engine/input/InputController.h
		ugine/engine/input/InputState.h
		ugine/engine/input/InputState.cpp
		ugine/engine/input/InputSystem.h
		ugine/engine/input/InputSystem.cpp

		ugine/engine/math/Aabb.h
		ugine/engine/math/Culling.cpp
		ugine/engine/math/Culling.h
		ugine/engine/math/Frustum.h
		ugine/engine/math/Math.cpp
		ugine/engine/math/Math.h
		ugine/engine/math/Poisson.cpp
		ugine/engine/math/Poisson.h
		ugine/engine/math/Raycast.cpp
		ugine/engine/math/Raycast.h

		ugine/engine/script/Component.h
		ugine/engine/script/NativeScript.cpp
		ugine/engine/script/NativeScript.h
		ugine/engine/script/Lua.cpp
		ugine/engine/script/Lua.h
		ugine/engine/script/LuaScript.cpp
		ugine/engine/script/LuaScript.h
		ugine/engine/script/ScriptSystem.cpp
		ugine/engine/script/ScriptSystem.h
		ugine/engine/script/Script.h
		ugine/engine/script/ScriptState.cpp
		ugine/engine/script/ScriptState.h
		ugine/engine/script/ScriptScene.cpp
		ugine/engine/script/ScriptScene.h

		ugine/engine/system/Platform.h
		ugine/engine/system/Event.h		

		ugine/engine/utils/CameraController.cpp
		ugine/engine/utils/CameraController.h
 )

 add_library(uGine::uGine ALIAS uGine)

 target_include_directories(
	uGine
	PRIVATE
		${GENERATED_DIR}
)

if (WIN32)
target_sources(
	uGine
	PRIVATE
		ugine/engine/system/win32/Win32Platform.cpp
		ugine/engine/system/win32/Win32Platform.h
		ugine/engine/system/win32/Win32Window.cpp
		ugine/engine/system/win32/Win32Window.h
		ugine/engine/system/win32/DirectInput.h
		ugine/engine/system/win32/DirectInput.cpp
		ugine/engine/system/win32/DirectInputDevice.h
		ugine/engine/system/win32/DirectInputDevice.cpp
)
endif ()

target_include_directories(
	uGine
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_BINARY_DIR}
)

target_link_libraries(
	uGine
	PRIVATE
		sourceEmbed::Headers
)

target_link_libraries(
	uGine
	PUBLIC
		uGine::CompilerFlags
		uGine::Foundation
		uGine::Gfx

		IconFontCppHeaders
		ImGui

		libluajit
		LuaBridge
)
