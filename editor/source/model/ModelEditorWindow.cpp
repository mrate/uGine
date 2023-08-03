#include "ModelEditorWindow.h"
#include "../EditorContext.h"
#include "../widgets/PropertyTable.h"
#include "../widgets/ResourceSelectWidget.h"

#include <ugine/Profile.h>
#include <ugine/engine/gfx/Component.h>
#include <ugine/engine/math/Math.h>

#include <glm/gtx/transform.hpp>

namespace ugine::ed {

ModelEditorWindow::ModelEditorWindow(EditorContext& context)
    : EditorTool{ context, false }
    , preview_{ context, true } {

    auto& view{ context_.MainMenu().Get("View") };
    view.AddAction([this] { return ImGui::Checkbox("Model editor", &visible_); });

    context_.Events().Connect<OpenResourceEvent<Model>, &ModelEditorWindow::OnOpenResource>(this);
    context_.Events().Connect<OpenTypelessResourceEvent, &ModelEditorWindow::OnOpenTypelessResource>(this);

    preview_.SetClearColor(ColorRGBA{ 0.1f, 0.1f, 0.8f, 1.0f });

    testSocketGO_ = preview_.World().CreateObject("SocketTest");
    testSocketGO_.CreateComponent<MeshComponent>();
    testSocketGO_.SetEnabled(false);

    ResetCamera();
}

void ModelEditorWindow::SetModel(ResourceHandle<Model> model) {
    UGINE_ASSERT(model);

    model_ = model;
    modelReady_ = model_->Ready();

    previewAnimation_ = context_.Assets().EmptyAnimation;
    animationPlay_ = false;
    isSkinned_ = false;
    visible_ = true;
    popup_ = false;
    selectedSocket_ = {};
    testSocketEnabled_ = false;
    testSocketGO_.SetEnabled(testSocketEnabled_);

    if (modelReady_) {
        UpdateModel();
    }
}

void ModelEditorWindow::Render() {
    PROFILE_EVENT_NC("ModelEditorWindow", COLOR_EDITOR_PROFILE);

    if (ImGui::Begin(ICON_FA_CUBE " Model editor", &visible_)) {
        RenderContent();
    }
    ImGui::End();

    preview_.SetRendering(visible_);
}

void ModelEditorWindow::RenderBoneTree(int& id, uint32_t i) {
    if (!isSkinned_) {
        return;
    }

    ImScope::Id _id{ &id };

    const auto& node{ model_->Nodes()[i] };
    const auto title{ std::format("{} {}", node.boneIndex == Model::INVALID_INDEX ? "" : ICON_FA_BONE, node.name.Data()) };

    int flags{ ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow };
    if (node.children.Empty()) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }

    const auto isBone{ node.boneIndex != Model::INVALID_INDEX };
    if (isBone && preview_.GetSelectedBone() == node.boneIndex) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    ImGui::PushStyleColor(ImGuiCol_Text, isBone ? ImVec4{ 1, 1, 1, 1 } : ImVec4{ 0.5f, 0.5f, 0.5f, 1 });
    const auto opened{ ImGui::TreeNodeEx(title.c_str(), flags) };
    ImGui::PopStyleColor();
    if (isBone && (ImGui::IsItemClicked(0) || ImGui::IsItemClicked(1))) {
        preview_.SelectBone(node.boneIndex);
        selectedSocket_ = {};
        if (ImGui::IsItemClicked(1)) {
            popup_ = 1;
        }
    }

    if (!opened) {
        return;
    }

    for (auto child : node.children) {
        RenderBoneTree(id, child);
    }

    for (const auto& socket : model_->Sockets()) {
        if (socket.boneIndex != node.boneIndex) {
            continue;
        }

        ImScope::Color text{ ImGuiCol_Text, ImVec4{ 0.8f, 0.2f, 0.2f, 1 } };

        int flags{ ImGuiTreeNodeFlags_Leaf };
        if (selectedSocket_ == socket.id) {
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        if (ImGui::TreeNodeEx(std::format("{} {}", ICON_FA_LINK, socket.name.Data()).c_str(), flags)) {
            ImGui::TreePop();
        }

        if (ImGui::IsItemClicked(0) || ImGui::IsItemClicked(1)) {
            preview_.DeselectBone();
            selectedSocket_ = socket.id;
            if (ImGui::IsItemClicked(1)) {
                popup_ = 2;
            }
        }
    }

    ImGui::TreePop();
}

void ModelEditorWindow::RenderModelProperties() {
    if (!model_) {
        return;
    }

    {
        PropertyTable table{ "object", &context_ };

        auto showBones{ preview_.GetBonesVisible() };
        if (table.EditProperty("Show bones", showBones)) {
            preview_.SetBonesVisible(showBones);
        }

        table.ConstProperty("AABB min", model_->BoundingBox().Min());
        table.ConstProperty("AABB max", model_->BoundingBox().Max());
    }

    if (ImGui::BeginTabBar("Tabs")) {
        if (ImGui::BeginTabItem("Materials")) {
            RenderMaterialsTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Bones")) {
            int id{};
            RenderBoneTree(id, 0);

            switch (popup_) {
            case 1:
                ImGui::OpenPopup("BoneMenu");
                popup_ = 0;
                break;
            case 2:
                ImGui::OpenPopup("SocketMenu");
                popup_ = 0;
                break;
            }

            RenderPopups();

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Preview")) {
            RenderPreviewTab();

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

void ModelEditorWindow::RenderMaterialsTab() {
    int id{};
    PropertyTable table{ "Material slots: ", &context_ };
    for (u32 slot{}; auto material : model_->Materials()) {
        ImScope::Id _id{ &id };

        if (table.EditProperty(std::format("Material {}", id - 1).c_str(), material)) {
            model_->SetMaterial(slot, material);
        }

        ++slot;
    }
}

void ModelEditorWindow::RenderPopups() {
    if (ImGui::BeginPopup("BoneMenu")) {
        if (ImGui::Selectable("Add socket")) {
            // TODO:
            UGINE_ASSERT(preview_.GetSelectedBone() != Model::INVALID_INDEX);

            auto socketName{ std::format("Socket #{}", model_->Sockets().Size()) };
            model_->AddSocket(Model::Socket{
                .name = socketName.c_str(),
                .id = StringID{ socketName },
                .boneIndex = preview_.GetSelectedBone(),
            });
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("SocketMenu")) {
        if (ImGui::Selectable("Delete socket")) {
            model_->DeleteSocket(selectedSocket_);
            selectedSocket_ = {};
        }
        ImGui::EndPopup();
    }
}

void ModelEditorWindow::RenderPreviewTab() {
    PropertyTable table{ "preview", &context_ };

    UGINE_ASSERT(previewAnimation_);

    if (table.EditProperty("Animation", previewAnimation_)) {
        preview_.SetSkinnedModel(model_, previewAnimation_);
    }

    if (table.EditProperty("Play", animationPlay_)) {
        preview_.SetAnimationPlaying(animationPlay_);
    }

    if (animationPlay_) {
        ImGui::BeginDisabled(true);
    }

    auto animationTime{ preview_.GetAnimationTime() };
    if (table.EditProperty("Time", animationTime, 0.0f, previewAnimation_->lengthSeconds)) {
        preview_.SetAnimationTime(animationTime);
    }

    if (animationPlay_) {
        ImGui::EndDisabled();
    }

    ImGui::Separator();

    if (table.EditProperty("Test socket", testSocketEnabled_)) {
        testSocketGO_.SetEnabled(testSocketEnabled_);
    }

    if (!testSocketEnabled_) {
        ImGui::BeginDisabled();
    }

    auto model{ testSocketGO_.Component<MeshComponent>().modelInstance.GetModel() };
    if (table.EditProperty("Model", model)) {
        testSocketGO_.Patch<MeshComponent>([&](auto& comp) {
            comp.modelInstance.SetModel(model);
            comp.modelInstance.ResetMaterials();
        });
    }

    table.BeginProperty("Socket");

    const auto selectedSocket{ model_->Sockets().FindIf([&](const auto& socket) { return socket.id == testSocketName_; }) };

    const char* preview{ selectedSocket >= 0 ? model_->Sockets()[selectedSocket].name.Data() : "<none>" };
    if (ImGui::BeginCombo("##testsocketname", preview)) {
        for (auto& socket : model_->Sockets()) {
            bool selected{ socket.id == testSocketName_ };
            if (ImGui::Selectable(socket.name.Data(), &selected)) {
                testSocketName_ = socket.id;
            }
        }

        ImGui::EndCombo();
    }

    if (!testSocketEnabled_) {
        ImGui::EndDisabled();
    }
}

void ModelEditorWindow::UpdateModel() {
    isSkinned_ = !model_->Bones().Empty();
    if (isSkinned_) {
        preview_.SetSkinnedModel(model_, previewAnimation_);
        preview_.SetAnimationPlaying(animationPlay_);
        preview_.SetBonesVisible(true);
    } else {
        preview_.SetModel(model_);
    }
    ResetCamera();
}

void ModelEditorWindow::UpdateSocket() {
    if (!testSocketEnabled_) {
        return;
    }

    const auto it{ model_->Sockets().FindIf([&](const auto& socket) { return socket.id == testSocketName_; }) };
    if (it < 0) {
        return;
    }

    const auto& socket{ model_->Sockets()[it] };
    Transformation trans{ socket.offsetTransformation * preview_.GetBoneTransformation(socket.boneIndex) };
    trans.scale = glm::vec3{ 1.0f };
    testSocketGO_.SetGlobalTransformation(trans);
}

void ModelEditorWindow::ResetCamera() {
    if (model_) {
        cameraDistance_ = std::max(1.0f, model_->BoundingBox().Diagonal());
        cameraOffset_ = model_->BoundingBox().HalfSize();
    } else {
        cameraDistance_ = 1.0f;
        cameraOffset_ = {};
    }

    theta_ = 45.0f;
    phi_ = 45.0f;

    UpdateCamera();
}

void ModelEditorWindow::OnOpenResource(const OpenResourceEvent<Model>& resource) {
    SetModel(resource.resource);
}

void ModelEditorWindow::OnOpenTypelessResource(const OpenTypelessResourceEvent& event) {
    if (event.resource.type == Model::TYPE) {
        SetModel(context_.Engine().GetResources().Get<Model>(event.resource.id));
    }
}

void ModelEditorWindow::RenderContent() {
    const auto flags{ ImGuiTableFlags_Resizable };
    if (!ImGui::BeginTable("##ModelEditor", 2, flags)) {
        return;
    }
    
    ImGui::TableSetupColumn("##preview", ImGuiTableColumnFlags_WidthStretch);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    RenderPreview();

    ImScope::Disabled disabled{ !modelReady_ };

    ImGui::TableSetColumnIndex(1);
    if (ImGui::BeginChild("Model properties")) {
        RenderModelProperties();
    }
    ImGui::EndChild();

    if (selectedSocket_ != 0) {
        RenderSocketProperties();
    } else if (preview_.GetSelectedBone() != Model::INVALID_INDEX) {
        if (ImGui::BeginChild("Bone properties")) {
            auto tmp{ preview_.GetBoneTransformation(preview_.GetSelectedBone()) };
            ImGuiEx::TransformationEditor("Transformation", tmp);
        }

        ImGui::EndChild();
    }

    ImGui::EndTable();
}

void ModelEditorWindow::RenderSocketProperties() {
    if (ImGui::BeginChild("Socket properties")) {
        const auto index{ model_->Sockets().FindIf([&](const auto& socket) { return socket.id == selectedSocket_; }) };
        if (index >= 0) {
            // TODO: Transformation gizmo.
            auto socket{ model_->Sockets()[index] };

            Transformation tmp{ socket.offsetTransformation };
            if (ImGuiEx::TransformationEditor("Transformation", tmp)) {
                socket.offsetTransformation = tmp.Matrix();
                model_->UpdateSocket(index, socket);
            }
        }
    }

    ImGui::EndChild();
}

void ModelEditorWindow::RenderPreview() {
    if (ImGui::BeginChild("Model view")) {
        const auto size{ ImGui::GetContentRegionAvail() };
        if (size.x > 0 && size.y > 0) {
            Resize(u32(size.x), u32(size.y));

            const auto position{ ImGui::GetCursorScreenPos() };

            if (!modelReady_ && model_ && model_->Ready()) {
                modelReady_ = true;
                UpdateModel();
            }

            if (isSkinned_) {
                preview_.UpdateBones();
                UpdateSocket();
            }

            auto output{ preview_.GetOutput() };
            ImGui::Image(ImGuiTextureHandle(output, context_.GfxDevice()), ImVec2{ float(width_), float(height_) });

            UpdateCameraControl(position);
        }
    }
    ImGui::EndChild();
}

void ModelEditorWindow::UpdateCameraControl(const ImVec2& position) {
    const auto hover{ ImGui::IsMouseHoveringRect(position, ImVec2{ position.x + width_, position.y + height_ }) };
    const auto wheel{ ImGui::GetIO().MouseWheel };
    if (hover && wheel) {
        cameraDistance_ = std::max(0.5f, cameraDistance_ - wheel);

        UpdateCamera();
    }

    if (ImGui::IsWindowFocused()) {
        if (ImGui::IsMouseClicked(0) && hover) {
            controlling_ = Controlling::Rotation;
            delta_ = {};
        }

        if (ImGui::IsMouseClicked(1) && hover) {
            controlling_ = Controlling::Translation;
            delta_ = {};
            //ResetCamera();
        }

        if (controlling_ == Controlling::Rotation && ImGui::IsMouseReleased(0) || (controlling_ == Controlling::Translation && ImGui::IsMouseReleased(1))) {
            controlling_ = Controlling::None;
        }

        if (controlling_ == Controlling::Rotation) {
            const auto delta{ ImGui::GetMouseDragDelta(0) };

            theta_ += (delta_.x - delta.x) / 2.0f;
            phi_ += (delta_.y - delta.y) / 2.0f;
            phi_ = std::max(1.0f, std::min(179.0f, phi_));

            delta_ = delta;

            UpdateCamera();
        } else if (controlling_ == Controlling::Translation) {
            const auto delta{ ImGui::GetMouseDragDelta(1) };

            auto forward{ -GetCameraPosition() };
            forward = glm::normalize(glm::vec3{ forward.x, 0, forward.y });

            auto right{ glm::normalize(glm::cross(math::UP, forward)) };
            cameraOffset_ += 0.05f * ((delta_.y - delta.y) * (ImGui::GetIO().KeyShift ? math::UP : forward) + (delta_.x - delta.x) * right);

            delta_ = delta;

            UpdateCamera();
        }
    }
}

void ModelEditorWindow::Resize(uint32_t width, uint32_t height) {
    if (width == width_ && height_ == height) {
        return;
    }

    width_ = width;
    height_ = height;

    preview_.Resize(width, height);
}

void ModelEditorWindow::UpdateCamera() {
    preview_.SetCameraTransform(Transformation{ glm::inverse(glm::lookAtRH(cameraOffset_ + GetCameraPosition(), cameraOffset_, math::UP)) });
}

glm::vec3 ModelEditorWindow::GetCameraPosition() const {
    return SphericalToCartesian(cameraDistance_, glm::radians(theta_), glm::radians(phi_));
}

} // namespace ugine::ed