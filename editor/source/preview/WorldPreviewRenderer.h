#pragma once

#include <ugine/Vector.h>

#include <ugine/engine/gfx/Material.h>
#include <ugine/engine/gfx/Model.h>
#include <ugine/engine/world/World.h>

#include <gfxapi/Types.h>

namespace ugine::ed {

class EditorContext;

class WorldPreviewRenderer {
public:
    enum class MaterialPreview {
        Sphere,
        Cube,
        Plane,
        Skybox,
    };

    WorldPreviewRenderer(EditorContext& context, bool floor);
    ~WorldPreviewRenderer();

    void Resize(uint32_t width, uint32_t height);

    void SetRendering(bool rendering);
    void SetOutput(ugine::gfxapi::TextureHandle texture, const gfxapi::Extent2D& extent);
    ugine::gfxapi::TextureHandle GetOutput();

    void SetCameraTransform(const ugine::Transformation& transform);
    const ugine::Transformation& GetCameraTransform() const;

    void SetMaterialPreviewCamera();
    void SetModelPreviewCamera();

    void SetMaterial(ugine::ResourceHandle<ugine::Material> material);

    void SetModel(ugine::ResourceHandle<ugine::Model> model);

    void SetSkinnedModel(ugine::ResourceHandle<ugine::Model> model, ugine::ResourceHandle<ugine::Animation> animation);
    void SetAnimationPlaying(bool playing);
    float GetAnimationTime() const;
    void SetAnimationTime(float time);
    void SetClearColor(const ColorRGBA& color);
    void SetBonesVisible(bool show);
    bool GetBonesVisible() const;
    void SelectBone(uint32_t bone);
    void DeselectBone();
    uint32_t GetSelectedBone() const { return selectedBone_; }
    void UpdateBones();
    const ugine::Transformation& GetBoneTransformation(uint32_t bone) const;

    ugine::World& World() { return *previewWorld_; }

    ugine::GameObject ModelGO() const { return model_; }

    void SetMaterialPreviewModel(MaterialPreview model);
    MaterialPreview MaterialPreviewModel() const;

private:
    void InitBones(ugine::ResourceHandle<ugine::Model> model);
    void CameraPose(float distance, const glm::vec3& offset);

    EditorContext& context_;

    uint32_t width_{};
    uint32_t height_{};

    ugine::World* previewWorld_{};

    glm::vec3 modelCenter_{};
    float modelSize_{};

    GameObject cameraGO_;
    GameObject model_;
    GameObject material_;

    // Skinned mesh
    GameObject skinnedModel_;
    GameObject bonesGO_;
    Vector<ugine::GameObject> boneGO_;
    uint32_t selectedBone_{ uint32_t(-1) };

    ResourceHandle<ugine::Model> materialModel_;
    ResourceHandle<ugine::Model> materialSphere_;
    ResourceHandle<ugine::Model> materialCube_;
    ResourceHandle<ugine::Model> materialPlane_;
    ResourceHandle<ugine::Model> materialSkybox_;

    ResourceHandle<Material> previewMaterial_;
};

} // namespace ugine::ed