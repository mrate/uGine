#pragma once

#include <ugine/engine/core/Resource.h>
#include <ugine/engine/gfx/Animation.h>
#include <ugine/engine/gfx/Model.h>

namespace ugine::ed {

class EditorContext;

class EmbededAssets {
public:
    ResourceHandle<Model> GridModel;
    ResourceHandle<Model> BoneModel;
    ResourceHandle<Material> BoneMaterial;
    ResourceHandle<Material> SelectedBoneMaterial;
    ResourceHandle<Animation> EmptyAnimation;
    ResourceHandle<Texture> FileIcon;
    ResourceHandle<Texture> WorldIcon;
    ResourceHandle<Texture> AnimationIcon;
    ResourceHandle<Texture> ScriptIcon;
    ResourceHandle<Texture> ShaderIcon;

    EmbededAssets(EditorContext& context);
    ~EmbededAssets();

    ResourceHandle<Model> CreateSphere();
    ResourceHandle<Model> CreateCube();
    ResourceHandle<Model> CreatePlane();
    ResourceHandle<Model> CreateSkybox();

private:
    EditorContext& context_;
};

} // namespace ugine::ed