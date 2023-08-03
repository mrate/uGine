#include "WorldPreviewRenderer.h"
#include "../EditorContext.h"

#include <ugine/engine/gfx/Component.h>
#include <ugine/engine/gfx/GraphicsScene.h>
#include <ugine/engine/gfx/ImGui.h>
#include <ugine/engine/world/WorldManager.h>

namespace ugine::ed {

using namespace ugine::gfxapi;

WorldPreviewRenderer::WorldPreviewRenderer(EditorContext& context, bool floor)
    : context_{ context }
    , previewWorld_{ context.Engine().GetWorldManager().CreateWorld(WORLD_FLAGS_PREVIEW) }
    , boneGO_{ context.Allocator() } {

    previewWorld_->SetName("PreviewWorld");

    materialSphere_ = context_.Assets().CreateSphere();
    materialCube_ = context_.Assets().CreateCube();
    materialPlane_ = context_.Assets().CreatePlane();
    materialSkybox_ = context_.Assets().CreateSkybox();

    materialModel_ = materialSphere_;

    material_ = previewWorld_->CreateObject("MaterialModel");
    material_.CreateComponent<MeshComponent>(materialSphere_);
    material_.SetEnabled(true);

    model_ = previewWorld_->CreateObject("Model");
    model_.CreateComponent<MeshComponent>();
    model_.SetEnabled(false);

    skinnedModel_ = previewWorld_->CreateObject("SkinnedModel");
    skinnedModel_.CreateComponent<MeshComponent>();
    skinnedModel_.CreateComponent<AnimationControllerComponent>(context.Assets().EmptyAnimation);
    skinnedModel_.SetEnabled(false);

    bonesGO_ = previewWorld_->CreateObject("Bones");
    bonesGO_.SetEnabled(false);
    skinnedModel_.AddChild(bonesGO_);

    cameraGO_ = previewWorld_->CreateObject("Camera");
    cameraGO_.CreateComponent<CameraComponent>();
    SetMaterialPreviewCamera();

    auto lightGO{ previewWorld_->CreateObject("DirLight") };
    lightGO.SetGlobalTransformation(Transformation{ glm::vec3{ 0 }, glm::angleAxis(glm::pi<float>() / 2.0f, glm::vec3{ 1, -1, 1 }), glm::vec3{ 1 } });
    lightGO.CreateComponent<LightComponent>(LightComponent::Type::Directional);

    if (floor) {
        auto floorGO{ previewWorld_->CreateObject("Floor") };
        floorGO.CreateComponent<MeshComponent>(context.Assets().GridModel);
    }

    Resize(u32(context.ThumbnailSize().x), u32(context.ThumbnailSize().y));
}

WorldPreviewRenderer::~WorldPreviewRenderer() {
    context_.Engine().GetWorldManager().DestroyWorld(previewWorld_);
    previewWorld_ = nullptr;
}

void WorldPreviewRenderer::Resize(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;

    const Extent2D extent{ width_, height_ };

    cameraGO_.Patch<CameraComponent>([&](auto& camera) {
        camera.width = width_;
        camera.height = height_;
        camera.zNear = 0.001f;
        camera.zFar = 10000.0f;
        camera.clearColor = ColorRGBA{ 0.5f, 0.5f, 0.5f, 1 };
    });
}

void WorldPreviewRenderer::SetRendering(bool rendering) {
    previewWorld_->SetRendering(rendering);
}

void WorldPreviewRenderer::SetOutput(ugine::gfxapi::TextureHandle texture, const gfxapi::Extent2D& extent) {
    return previewWorld_->GetScene<GraphicsScene>()->SetCameraRtv(cameraGO_, texture, extent);
}

ugine::gfxapi::TextureHandle WorldPreviewRenderer::GetOutput() {
    return previewWorld_->GetScene<GraphicsScene>()->GetCameraRtv(cameraGO_);
}

void WorldPreviewRenderer::SetCameraTransform(const Transformation& transform) {
    cameraGO_.SetGlobalTransformation(transform);
}

const Transformation& WorldPreviewRenderer::GetCameraTransform() const {
    return cameraGO_.GlobalTransformation();
}

void WorldPreviewRenderer::SetMaterialPreviewCamera() {
    CameraPose(1.0f, glm::vec3{});
}

void WorldPreviewRenderer::SetModelPreviewCamera() {
    CameraPose(modelSize_, modelCenter_);
}

void WorldPreviewRenderer::SetModel(ResourceHandle<Model> model) {
    modelCenter_ = model->BoundingBox().CenterPoint();
    modelSize_ = glm::length(model->BoundingBox().Size()) / 2.0f;

    model_.Patch<MeshComponent>([&](auto& meshComponent) {
        meshComponent.modelInstance.SetModel(model);
        meshComponent.modelInstance.ResetMaterials();
    });
    model_.SetEnabled(true);
    skinnedModel_.SetEnabled(false);
    material_.SetEnabled(false);
    SetBonesVisible(false);
}

void WorldPreviewRenderer::SetSkinnedModel(ResourceHandle<Model> model, ResourceHandle<Animation> animation) {
    modelCenter_ = model->BoundingBox().CenterPoint();
    modelSize_ = glm::length(model->BoundingBox().Size()) / 2.0f;

    skinnedModel_.Patch<MeshComponent>([&](auto& meshComponent) { meshComponent.modelInstance.SetModel(model); });
    skinnedModel_.Patch<AnimationControllerComponent>([&](auto& animator) { animator.animation = animation; });
    model_.SetEnabled(false);
    skinnedModel_.SetEnabled(true);
    material_.SetEnabled(false);

    InitBones(model);
    DeselectBone();
}

float WorldPreviewRenderer::GetAnimationTime() const {
    return skinnedModel_.Component<AnimationControllerComponent>().animationTime;
}

void WorldPreviewRenderer::SetAnimationPlaying(bool playing) {
    skinnedModel_.Patch<AnimationControllerComponent>([&](auto& animator) { animator.isRunning = playing; });
}

void WorldPreviewRenderer::SetAnimationTime(float time) {
    skinnedModel_.Patch<AnimationControllerComponent>([&](auto& animator) { animator.animationTime = time; });
}

void WorldPreviewRenderer::SetMaterial(ResourceHandle<Material> material) {
    previewMaterial_ = material;
    material_.Patch<MeshComponent>([&](auto& meshComponent) {
        meshComponent.modelInstance.SetModel(materialModel_);
        meshComponent.modelInstance.SetMaterial(0, material);
    });
    model_.SetEnabled(false);
    skinnedModel_.SetEnabled(false);
    material_.SetEnabled(true);
    SetBonesVisible(false);
}

void WorldPreviewRenderer::SetClearColor(const ColorRGBA& color) {
    cameraGO_.Patch<CameraComponent>([&](auto& camera) { camera.clearColor = color; });
}

void WorldPreviewRenderer::SetBonesVisible(bool show) {
    bonesGO_.SetEnabled(show);
    for (auto& go : boneGO_) {
        go.SetEnabled(show);
    }
}

bool WorldPreviewRenderer::GetBonesVisible() const {
    return bonesGO_.IsEnabled();
}

void WorldPreviewRenderer::SelectBone(uint32_t node) {
    DeselectBone();

    selectedBone_ = node;
    boneGO_[selectedBone_].Patch<MeshComponent>([&](auto& m) { m.modelInstance.SetMaterial(0, context_.Assets().SelectedBoneMaterial); });
}

void WorldPreviewRenderer::DeselectBone() {
    if (selectedBone_ != Model::INVALID_INDEX) {
        boneGO_[selectedBone_].Patch<MeshComponent>([&](auto& m) { m.modelInstance.ResetMaterials(); });
        selectedBone_ = Model::INVALID_INDEX;
    }
}

void WorldPreviewRenderer::UpdateBones() {
    auto model{ skinnedModel_.Component<MeshComponent>().modelInstance.GetModel() };
    auto& animation{ skinnedModel_.Component<AnimationControllerComponent>().animation };
    auto animationTime{ skinnedModel_.Component<AnimationControllerComponent>().animationTime };

    if (!model || !animation) {
        return;
    }

    Vector<glm::mat4> matrices{ model->Bones().Size(), context_.FrameAllocator() };
    UpdateAnimation(context_.Allocator(), *model.Get(), *animation.Get(), animationTime, matrices.ToSpan());

    Vector<Transformation> transformations(model->Bones().Size(), context_.FrameAllocator());

    const auto rootTransform{ model->Meshes().Empty() ? glm::mat4{ 1.0f } : model->Meshes()[0].transformation };
    for (uint32_t i{}; i < model->Bones().Size(); ++i) {
        transformations[i] = Transformation{ model->RootTransform() * rootTransform * matrices[i] * glm::inverse(model->Bones()[i].offsetMatrix) };
    }

    for (uint32_t i{}; i < model->Bones().Size(); ++i) {
        auto transformation{ transformations[i] };
        const auto bonePosition{ transformation.position };
        const auto parentPosition{ model->Bones()[i].parentBone != Model::INVALID_INDEX ? transformations[model->Bones()[i].parentBone].position
                                                                                        : glm::vec3{ bonePosition.x, 0, bonePosition.z } };
        const auto distance{ glm::length(parentPosition - bonePosition) };

        transformation.position = (bonePosition + parentPosition) / 2.0f;
        transformation.rotation = glm::rotation(math::FORWARD, glm::normalize(parentPosition - bonePosition));
        transformation.scale = glm::vec3{ distance };

        boneGO_[i].SetGlobalTransformation(transformation);
    }
}

const Transformation& WorldPreviewRenderer::GetBoneTransformation(uint32_t bone) const {
    return boneGO_[bone].GlobalTransformation();
}

void WorldPreviewRenderer::SetMaterialPreviewModel(MaterialPreview model) {
    auto material{ materialModel_->GetMaterial(0) };

    switch (model) {
    case MaterialPreview::Sphere: materialModel_ = materialSphere_; break;
    case MaterialPreview::Cube: materialModel_ = materialCube_; break;
    case MaterialPreview::Plane: materialModel_ = materialPlane_; break;
    case MaterialPreview::Skybox: materialModel_ = materialSkybox_; break;
    }

    materialModel_->SetMaterial(0, material);

    material_.Patch<MeshComponent>([&](auto& meshComponent) {
        meshComponent.modelInstance.SetModel(materialModel_);
        meshComponent.modelInstance.SetMaterial(0, previewMaterial_);
    });
}

WorldPreviewRenderer::MaterialPreview WorldPreviewRenderer::MaterialPreviewModel() const {
    if (materialModel_ == materialSphere_) {
        return MaterialPreview::Sphere;
    } else if (materialModel_ == materialCube_) {
        return MaterialPreview::Cube;
    } else if (materialModel_ == materialPlane_) {
        return MaterialPreview::Plane;
    } else {
        return MaterialPreview::Skybox;
    }
}

void WorldPreviewRenderer::InitBones(ResourceHandle<Model> model) {
    if (model->Nodes().Empty()) {
        return;
    }

    for (auto bone : boneGO_) {
        GameObject::Destroy(bone);
    }
    boneGO_.Clear();

    if (boneGO_.Size() < model->Bones().Size()) {
        auto size{ boneGO_.Size() };
        boneGO_.Resize(model->Bones().Size());

        for (size_t i{ size }; i < boneGO_.Size(); ++i) {
            boneGO_[i] = previewWorld_->CreateObject(model->Bones()[i].name.Data());
            boneGO_[i].CreateComponent<MeshComponent>(context_.Assets().BoneModel);
            bonesGO_.AddChild(boneGO_[i]);
        }
    }

    UpdateBones();
}

void WorldPreviewRenderer::CameraPose(float distance, const glm::vec3& offset) {
    const auto cameraPosition{ SphericalToCartesian(distance, glm::radians(45.0f), glm::radians(45.0f)) };

    SetCameraTransform(Transformation{ glm::inverse(glm::lookAtRH(offset + cameraPosition, offset, math::UP)) });
}

} // namespace ugine::ed