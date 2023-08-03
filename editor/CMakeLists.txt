project(uGineEditor VERSION 0.0.5)

set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../uGine/ugineEngine/ugine/engine/shaders)
set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)

embed_shader_resource(assets/materials/grid.shader ${GENERATED_DIR} grid_shader grid_shader ${INCLUDE_DIR})
embed_shader_resource(assets/materials/bone.shader ${GENERATED_DIR} bone_shader bone_shader ${INCLUDE_DIR})

embed_material(assets/materials/grid.mat ${GENERATED_DIR} grid_material grid_material ${INCLUDE_DIR})
embed_material(assets/materials/bone.mat ${GENERATED_DIR} bone_material bone_material ${INCLUDE_DIR})

configure_file(ugineEditorConfig.h.in ugineEditorConfig.h)

add_executable(
	${PROJECT_NAME}
		${GENERATED_DIR}/grid_shader.cpp
		${GENERATED_DIR}/grid_shader.h
		${GENERATED_DIR}/grid_material.cpp
		${GENERATED_DIR}/grid_material.h
		
		${GENERATED_DIR}/bone_shader.cpp
		${GENERATED_DIR}/bone_shader.h
		${GENERATED_DIR}/bone_material.cpp
		${GENERATED_DIR}/bone_material.h

		source/Editor.cpp
		source/EditorContext.cpp
		source/EditorContext.h
		source/EditorSettings.cpp
		source/EditorSettings.h
		source/EditorScene.cpp
		source/EditorScene.h
		source/EditorStyle.cpp
		source/MetaFile.cpp
		source/MetaFile.h
		
		source/world/Actions.cpp
		source/world/Actions.h
		source/world/CameraPreview.cpp
		source/world/CameraPreview.h
		source/world/EditorViewport.cpp
		source/world/EditorViewport.h
		source/world/GameObjectEditor.cpp
		source/world/GameObjectEditor.h
		source/world/GameViewport.cpp
		source/world/GameViewport.h
		source/world/WorldEditor.cpp
		source/world/WorldEditor.h
		source/world/WorldTool.cpp
		source/world/WorldTool.h
		
		source/platform/FileDialog.cpp
		source/platform/FileDialog.h

		source/assets/Assets.cpp
		source/assets/AssetsMonitor.cpp
		source/assets/AssetsMonitor.h
		source/assets/EmbededAssets.h
		source/assets/EmbededAssets.cpp
		source/assets/AssetsBrowser.h
		source/assets/AssetsBrowser.cpp
		source/assets/AssetRenameDialog.h
		source/assets/AssetRenameDialog.cpp
		source/assets/AssetRegistry.h
		source/assets/AssetRegistry.cpp

		source/tools/CVarWindow.h
		source/tools/CVarWindow.cpp
		source/tools/LogWidget.cpp
		source/tools/LogWidget.h
		source/tools/StatsWidget.cpp
		source/tools/StatsWidget.h
		
		source/widgets/Menu.h
		source/widgets/Menu.cpp
		source/widgets/DirectoryTree.cpp
		source/widgets/DirectoryTree.h
		source/widgets/DragAndDrop.h		
		source/widgets/FilesystemResourceBrowser.cpp
		source/widgets/FilesystemResourceBrowser.h
		source/widgets/TransformationWidget.h
		source/widgets/TransformationWidget.cpp
		source/widgets/PropertyTable.h
		source/widgets/ResourceSelectWidget.cpp
		source/widgets/ResourceSelectWidget.h
		
		source/widgets/ImGuiScope.h
		source/widgets/ImGuiEx.cpp
		source/widgets/ImGuiEx.h
		
		source/model/AssimpHelper.cpp
		source/model/AssimpHelper.h
		source/model/MeshImporter.cpp
		source/model/MeshImporter.h
		source/model/ImportModelWindow.h
		source/model/ImportModelWindow.cpp
		source/model/ModelEditorWindow.h
		source/model/ModelEditorWindow.cpp
		source/model/ModelTool.cpp

		source/shader/ImportShaderWindow.h
		source/shader/ImportShaderWindow.cpp
		source/shader/ShaderPreviewWindow.h
		source/shader/ShaderPreviewWindow.cpp
		source/shader/ShaderTool.cpp
		source/shader/ShaderTool.h

		source/material/MaterialTool.cpp
		source/material/MaterialTool.h
		source/material/MaterialCreator.cpp
		source/material/MaterialCreator.h
		source/material/ImportMaterialWindow.cpp
		source/material/ImportMaterialWindow.h
		source/material/MaterialPreviewRenderer.h
		source/material/MaterialPreviewRenderer.cpp
		source/material/MaterialEditorWindow.h
		source/material/MaterialEditorWindow.cpp
		source/material/MaterialInstanceWindow.h
		source/material/MaterialInstanceWindow.cpp

		source/texture/TextureTool.cpp
		source/texture/TextureTool.h
		source/texture/ImportTextureWindow.h
		source/texture/ImportTextureWindow.cpp
		source/texture/PreviewTextureWindow.cpp
		source/texture/PreviewTextureWindow.h

		source/preview/WorldPreviewRenderer.h
		source/preview/WorldPreviewRenderer.cpp
		source/preview/ResourceThumbnails.h
		source/preview/ResourceThumbnails.cpp

		source/script/ScriptTool.cpp
		source/script/ScriptTool.h
		source/script/ScriptingWindow.h
		source/script/ScriptingWindow.cpp
		source/script/ScriptEditorWindow.h
		source/script/ScriptEditorWindow.cpp

		source/window/EditorWindow.cpp
		source/window/EditorWindow.h
		source/window/Window.cpp
		source/window/Window.h
		source/window/SaveWindow.h
		source/window/SaveWindow.cpp

		source/animation/AnimationTool.cpp
		source/animation/AnimationEditorWindow.h
		source/animation/AnimationEditorWindow.cpp
)

if (MSVC)
	target_compile_definitions(
		${PROJECT_NAME}
		PRIVATE
			_CRT_SECURE_NO_WARNINGS
	)
endif ()

target_include_directories(
	${PROJECT_NAME}
	PRIVATE
		${GENERATED_DIR}
		${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(
	${PROJECT_NAME}
	PRIVATE
		uGine::uGine
		uGine::materialImporter
		uGine::materialBuilder
		uGine::shaderImporter
		sourceEmbed::Headers

		assimp::assimp
		ImGuizmo
)

# TODO: Don't do that, use shaders from soruce directory when debugging.
# TODO: Copy shaders only on final build and install.
# Copy content of material shaders to editor output.
#add_custom_command(
#	TARGET ${PROJECT_NAME} POST_BUILD
#	COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/../uGine/ugineEngine/ugine/engine/shaders" $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
#)

add_custom_command(
	TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/assets/textures" $<TARGET_FILE_DIR:${PROJECT_NAME}>/editor
)