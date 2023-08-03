#include "GameObjectEditor.h"

#include "../EditorContext.h"
#include "../widgets/PropertyTable.h"

#include <ugine/engine/World/Camera.h>
#include <ugine/engine/gfx/GraphicsScene.h>
#include <ugine/engine/gfx/ImGui.h>

#include <ugine/Profile.h>

#include <glm/gtx/transform.hpp>

// TODO:
#include <format>

namespace ugine::ed {

#define EDIT_PROPERTY(table, name, type, component, member, ...)                                                                                               \
    {                                                                                                                                                          \
        auto member{ component.member };                                                                                                                       \
        if (table.EditProperty(name, member, __VA_ARGS__)) {                                                                                                   \
            go.Patch<type>([&](auto& c) { c.member = member; });                                                                                               \
        }                                                                                                                                                      \
    }

#define EDIT_PROPERTY_COMBO(table, name, type, component, member, names)                                                                                       \
    {                                                                                                                                                          \
        auto member{ component.member };                                                                                                                       \
        if (table.EditPropertyCombo(name, member, names)) {                                                                                                    \
            go.Patch<type>([&](auto& c) { c.member = member; });                                                                                               \
        }                                                                                                                                                      \
    }

namespace {

    std::map<const char*, Camera::ProjectionType> CameraProjections = {
        { "Perspective", Camera::ProjectionType::Perspective },
        { "Ortho", Camera::ProjectionType::Ortho },
    };

    std::map<const char*, LightComponent::Type> LightTypes = {
        { "Directional", LightComponent::Type::Directional },
        { "Spot", LightComponent::Type::Spot },
        { "Point", LightComponent::Type::Point },
    };

    //
    //std::map<const char*, vk::SampleCountFlagBits> Samples = {
    //    { "1x", vk::SampleCountFlagBits::e1 },
    //    { "2x", vk::SampleCountFlagBits::e2 },
    //    { "4x", vk::SampleCountFlagBits::e4 },
    //    { "8x", vk::SampleCountFlagBits::e8 },
    //    { "16x", vk::SampleCountFlagBits::e16 },
    //};
    //
    //std::map<const char*, RigidBodyComponent::Shape> RigidBodyShapes = {
    //    { "Box", RigidBodyComponent::Shape::Box },
    //    { "Sphere", RigidBodyComponent::Shape::Sphere },
    //    { "Plane", RigidBodyComponent::Shape::Plane },
    //}; // namespace

    template <typename T> void AddComponent(GameObject& go, const char* name) {
        if (!go.Has<T>()) {
            if (ImGui::Selectable(name)) {
                go.CreateComponent<T>();
            }
        }
    }

    template <typename T, typename F> void AddComponent(GameObject& go, const char* name, F&& f) {
        if (!go.Has<T>()) {
            if (ImGui::Selectable(name)) {
                f();
            }
        }
    }

    template <typename T> void RemoveComponent(GameObject& go, const char* name) {
        if (go.Has<T>()) {
            if (ImGui::Selectable(name)) {
                go.RemoveComponent<T>();
            }
        }
    }

} // namespace

GameObjectEditor::GameObjectEditor(EditorContext& context)
    : EditorTool{ context } {
    auto& view{ context_.MainMenu().Get("View") };

    view.AddAction([this] { return ImGui::Checkbox("Object editor", &visible_); });
}

void GameObjectEditor::Populate(GameObject& go, bool open) const {
    int flags{};

    if (open) {
        flags |= ImGuiTreeNodeFlags_DefaultOpen;
    }

    // Add / Remove component.
    if (ImGui::Button(ICON_FA_PLUS " Add component")) {
        ImGui::OpenPopup("AddComponent");
    }

    if (ImGui::BeginPopup("AddComponent")) {
        AddComponent<CameraComponent>(go, ICON_FA_CAMERA " Camera", [&]() {
            go.CreateComponent<CameraComponent>(CameraComponent{
                .isMain = false,
                .projection = Camera::ProjectionType::Perspective,
                .vFovDeg = 60.0f,
                .zNear = 0.1f,
                .zFar = 100.0f,
                .width = 100,
                .height = 100,
            });
        });
        AddComponent<LightComponent>(go, ICON_FA_LIGHTBULB " Light");
        AddComponent<MeshComponent>(go, ICON_FA_CUBE_OUTLINE " Mesh");
        AddComponent<NativeScriptComponent>(go, ICON_FA_CLIPBOARD " Native script");
        AddComponent<LuaScriptComponent>(go, ICON_FA_CLIPBOARD " Lua script");
        AddComponent<AnimationControllerComponent>(go, ICON_FA_FILM " Animation controller");
        AddComponent<SkyComponent>(go, ICON_FA_CLOUD " Sky component");

        //AddComponent<core::ParticleComponent>(go, ICON_FA_ATOM " Particle system");
        //AddComponent<core::RigidBodyComponent>(go, ICON_FA_CUBES " Rigid body");
        //AddComponent<core::TerrainComponent>(go, ICON_FA_MOUNTAIN " Terrain");
        //AddComponent<core::FoliageComponent>(go, ICON_FA_TREE " Foliage");

        ImGui::EndPopup();
    }

    // Edit components.
    if (ImGui::CollapsingHeader(ICON_FA_TAG " Tag", flags)) {
        ImScope::Indent i;
        Populate(go, go.Component<TagComponent>());
    }

    if (go.Has<TransformationComponent>()) {
        if (ImGui::CollapsingHeader(ICON_FA_AXIS_ARROW " Transformation component", flags)) {
            ImScope::Indent i;
            Populate(go, go.Component<TransformationComponent>());
        }
    }

    RenderComponent<CameraComponent>(go, ICON_FA_CAMERA " Camera component", flags);
    RenderComponent<LightComponent>(go, ICON_FA_LIGHTBULB " Light component", flags);
    RenderComponent<MeshComponent>(go, ICON_FA_CUBE_OUTLINE " Mesh component", flags);
    RenderComponent<NativeScriptComponent>(go, ICON_FA_CLIPBOARD " Native script component", flags);
    RenderComponent<LuaScriptComponent>(go, ICON_FA_CLIPBOARD " Lua script component", flags);
    RenderComponent<AnimationControllerComponent>(go, ICON_FA_FILM " Animation controller", flags);
    RenderComponent<SkyComponent>(go, ICON_FA_CLOUD " Sky component", flags);

    //RenderComponent<core::ParticleComponent>(go, ICON_FA_ATOM " Particle system component", flags);
    //RenderComponent<core::RigidBodyComponent>(go, ICON_FA_CUBES " Rigid body component", flags);
    //RenderComponent<core::TerrainComponent>(go, ICON_FA_MOUNTAIN " Terrain component", flags);
    //RenderComponent<core::FoliageComponent>(go, ICON_FA_TREE " Foliage component", flags);
}

void GameObjectEditor::Render() {
    PROFILE_EVENT_NC("Object editor", COLOR_EDITOR_PROFILE);

    if (ImGui::Begin("Object inspector")) {
        if (context_.SelectedGO().IsValid()) {
            Populate(context_.SelectedGO());
        }
    }
    ImGui::End();
}

void GameObjectEditor::Populate(GameObject& go, const TagComponent& tag) const {
    auto changed{ false };

    PropertyTable table("Tag");

    {
        auto name{ go.Name() };
        if (table.EditProperty("Name", name)) {
            go.SetName(name);
        }
    }

    bool enabled{ go.IsEnabled() };
    if (table.EditProperty("Enabled", enabled)) {
        go.SetEnabled(enabled);
    }

    bool isStatic{ go.IsStatic() };
    if (table.EditProperty("Static", isStatic)) {
        go.SetStatic(isStatic);
    }
}

void GameObjectEditor::Populate(GameObject& go, const TransformationComponent& trans) const {
    auto newTrans{ trans };

    ImGui::TextUnformatted("Global:");

    if (ImGuiEx::TransformationEditor("Transformation (global)", newTrans.globalTransformation)) {
        go.SetGlobalTransformation(newTrans.globalTransformation);
    }

    ImGui::TextUnformatted("Local:");

    if (ImGuiEx::TransformationEditor("Transformation (local)", newTrans.localTransformation)) {
        go.SetGlobalTransformation(newTrans.localTransformation);
    }
}

void GameObjectEditor::Populate(GameObject& go, const SkyComponent& comp) const {
    PropertyTable table{ "Camera", &context_ };

    EDIT_PROPERTY(table, "Material", SkyComponent, comp, material);
}

void GameObjectEditor::Populate(GameObject& go, const CameraComponent& cameraC) const {
    PropertyTable table{ "Camera", &context_ };

    EDIT_PROPERTY(table, "Clear color", CameraComponent, cameraC, clearColor);

    auto projection{ cameraC.projection };
    if (table.EditPropertyCombo("Projection", projection, CameraProjections)) {
        go.Patch<CameraComponent>([&](auto& c) { c.projection = projection; });
    }

    EDIT_PROPERTY(table, "Vertical fov", CameraComponent, cameraC, vFovDeg);
    EDIT_PROPERTY(table, "Near", CameraComponent, cameraC, zNear);
    EDIT_PROPERTY(table, "Far", CameraComponent, cameraC, zFar);
    EDIT_PROPERTY(table, "Main", CameraComponent, cameraC, isMain);

    {
        auto gfxScene{ context_.ActiveWorld()->GetScene<GraphicsScene>() };
        auto ao{ gfxScene->GetAoTextureIndex(go) };

        if (ao >= 0) {
            const auto imTex{ ImGuiTexture(ao, ImFlags::Depth) };

        table.BeginProperty("SSAO");
        ImGui::Image(imTex, ImVec2{ 128, 128 });
        ImGuiEx::ToolTip([&] { ImGui::Image(imTex, ImVec2{ 512, 512 }); });

        table.EndProperty();
        }
    }
}

void GameObjectEditor::Populate(GameObject& go, const LuaScriptComponent& comp) const {
    PropertyTable table{ "Lua script component", &context_ };

    EDIT_PROPERTY(table, "Script", LuaScriptComponent, comp, script);
}

void GameObjectEditor::Populate(GameObject& go, const MeshComponent& comp) const {
    PropertyTable table{ "Mesh component", &context_ };

    //EDIT_PROPERTY(table, "Model", MeshComponent, comp, model);
    {
        auto model{ comp.modelInstance.GetModel() };
        if (table.EditProperty("Model", model)) {
            go.Patch<MeshComponent>([&](auto& c) { c.modelInstance.SetModel(model); });
        }
    }

    for (u32 index{}; auto material : comp.modelInstance.GetMaterials()) {
        const auto i{ index++ };

        if (table.EditProperty(std::format("Material {}", i).c_str(), material)) {
            go.Patch<MeshComponent>([&](auto& c) { c.modelInstance.SetMaterial(i, material); });
        }
    }

    EDIT_PROPERTY(table, "Instanced", MeshComponent, comp, instanced);

    if (comp.instanced) {
        u32 instanceCount{ u32(comp.instanceTransformations.size()) };
        if (table.EditProperty("Instance count", instanceCount)) {
            go.Patch<MeshComponent>([&](auto& c) { c.instanceTransformations.resize(instanceCount); });
        }

        int id{};
        for (u32 i{}; i < comp.instanceTransformations.size(); ++i) {
            ImScope::Id _id{ &id };

            Transformation t{ ToMatrix(comp.instanceTransformations[i]) };
            table.BeginProperty("Transformation %d", i);
            if (ImGuiEx::TransformationEditor("##Transformation", t)) {
                go.Patch<MeshComponent>([&](auto& c) { c.instanceTransformations[i] = MaterialVertexInstanceFromMatrix(t.Matrix()); });
            }
        }
    }
}

void GameObjectEditor::Populate(GameObject& go, const NativeScriptComponent& script) const {
    PropertyTable table("Native script component");

    table.BeginProperty("Scripts:");
    ImGui::Text("Count: %u", script.Scripts().size());

    //if (ImGui::Button(ICON_FA_PLUS)) {
    //    ImGui::OpenPopup("AddNativeScript");
    //}

    //if (ImGui::BeginPopup("AddNativeScript")) {
    //    for (const auto& [name, r] : script::NativeScripts::Instance().Registered()) {
    //        if (ImGui::Selectable(name.c_str())) {
    //            go.Patch<NativeScriptComponent>([&](auto& c) { c.Add(go, script::NativeScripts::Instance().Create(name)); });
    //        }
    //    }

    //    ImGui::EndPopup();
    //}

    table.EndProperty();

    std::shared_ptr<NativeScript> toRemove;
    for (const auto& t : script.Scripts()) {
        table.BeginProperty("Script");

        ImGui::Text(t->Name());
        ImGui::SameLine();

        if (ImGui::Button(ICON_FA_MINUS)) {
            toRemove = t;
        }

        table.EndProperty();
    }

    if (toRemove) {
        go.Patch<NativeScriptComponent>([&](auto& c) { c.Remove(go, toRemove); });
        toRemove = {};
    }
}

void GameObjectEditor::Populate(GameObject& go, const LightComponent& comp) const {
    PropertyTable table{ "Light", &context_ };

    const auto hasShadowMap{ comp.generatesShadows };

    bool compChanged{};

    bool lightChanged{};

    auto newComp{ comp };

    lightChanged |= table.EditPropertyCombo("Type", newComp.type, LightTypes);
    lightChanged |= table.EditProperty("Color", newComp.color);
    lightChanged |= table.EditProperty("Intensity", newComp.intensity, 0.0f, 20.0f);

    if (comp.type != LightComponent::Type::Directional) {
        lightChanged |= table.EditProperty("Range", newComp.range, 0.0f, 10.0f);
    }

    if (comp.type == LightComponent::Type::Spot) {
        lightChanged |= table.EditProperty("Spot angle", newComp.spotAngleDeg, 0.01f, 90.0f);
    }

    //if (comp.type == LightComponent::Type::Directional) {
    //    EDIT_PROPERTY(table, "Cascaded shadow map", LightComponent, comp, isCsm);
    //}

    EDIT_PROPERTY(table, "Generate shadows", LightComponent, comp, generatesShadows);
    
    //EDIT_PROPERTY(table, "Flare texture", LightComponent, comp, flareTexture);
    //EDIT_PROPERTY(table, "Flare size", LightComponent, comp, flareSize);
    //EDIT_PROPERTY(table, "Flare fade", LightComponent, comp, flareFade, 0.001f, 0.01f);

    if (hasShadowMap) {
        auto gfxScene{ context_.ActiveWorld()->GetScene<GraphicsScene>() };
        auto shadowMap{ gfxScene->GetShadowMap(go) };
        const auto imTex{ ImGuiTextureHandle(shadowMap, context_.GfxDevice(), ImFlags::Depth) };

        table.BeginProperty("Shadow map");
        ImGui::Image(imTex, ImVec2{ 128, 128 });
        ImGuiEx::ToolTip([&] { ImGui::Image(imTex, ImVec2{ 512, 512 }); });

        table.EndProperty();
    }

    if (lightChanged) {
        go.Patch<LightComponent>([&](auto& c) { c = newComp; });
    }
}

void GameObjectEditor::Populate(GameObject& go, const AnimationControllerComponent& animator) const {
    PropertyTable table("Animator", &context_);

    const auto animationLength{ animator.animation ? animator.animation->lengthSeconds : 0.0f };

    EDIT_PROPERTY(table, "Animation", AnimationControllerComponent, animator, animation);
    EDIT_PROPERTY(table, "Time", AnimationControllerComponent, animator, animationTime, 0.0f, animationLength);
    EDIT_PROPERTY(table, "Running", AnimationControllerComponent, animator, isRunning);
    EDIT_PROPERTY(table, "Speed", AnimationControllerComponent, animator, speed, 0.0f, 50.0f);
    EDIT_PROPERTY(table, "Resolution", AnimationControllerComponent, animator, resolutionS, 0.001f, 10.0f);
}

//
//void GameObjectEditor::Populate(core::GameObject& go, const core::ParticleComponent& particle) const {
//    PropertyTable table("Particles", gui_, images_);
//
//    EDIT_PROPERTY(table, "Count", core::ParticleComponent, particle, count);
//    EDIT_PROPERTY(table, "Texture", core::ParticleComponent, particle, texture);
//    EDIT_PROPERTY(table, "Color", core::ParticleComponent, particle, color);
//    EDIT_PROPERTY(table, "Size", core::ParticleComponent, particle, size);
//    EDIT_PROPERTY(table, "Speed", core::ParticleComponent, particle, speed);
//    EDIT_PROPERTY(table, "Gravity", core::ParticleComponent, particle, gravity);
//    EDIT_PROPERTY(table, "Offset", core::ParticleComponent, particle, startOffset);
//    EDIT_PROPERTY(table, "Speed offset", core::ParticleComponent, particle, speedOffset);
//    EDIT_PROPERTY(table, "Lifetime", core::ParticleComponent, particle, life);
//    EDIT_PROPERTY(table, "Life span", core::ParticleComponent, particle, lifeSpan);
//    EDIT_PROPERTY(table, "Emit count", core::ParticleComponent, particle, emitCount);
//    EDIT_PROPERTY(table, "Mesh", core::ParticleComponent, particle, mesh);
//}
//
//void GameObjectEditor::Populate(core::GameObject& go, const core::RigidBodyComponent& body) const {
//    PropertyTable table("Rigid body");
//
//    EDIT_PROPERTY_COMBO(table, "Shape", core::RigidBodyComponent, body, shape, RigidBodyShapes);
//
//    auto scale{ body.scale };
//    if (table.EditProperty("Scale", scale, 0.001f, 1000.0f)) {
//        if (scale.x > 0 && scale.y > 0 && scale.z > 0) {
//            go.Patch<core::RigidBodyComponent>([&](auto& a) { a.scale = scale; });
//        }
//    }
//
//    EDIT_PROPERTY(table, "Offset", core::RigidBodyComponent, body, offset);
//    EDIT_PROPERTY(table, "Linear velocity", core::RigidBodyComponent, body, linearVelocity);
//    EDIT_PROPERTY(table, "Angular velocity", core::RigidBodyComponent, body, angularVelocity);
//    EDIT_PROPERTY(table, "Dynamic", core::RigidBodyComponent, body, isDynamic);
//    EDIT_PROPERTY(table, "Kinematic", core::RigidBodyComponent, body, isKinematic);
//}
//
//void GameObjectEditor::Populate(core::GameObject& go, const core::TerrainComponent& terrain) const {
//    PropertyTable table("Terrain", gui_, images_);
//
//    EDIT_PROPERTY(table, "Height map", core::TerrainComponent, terrain, heightMap, TEXTURE_FILES_FILTER);
//    EDIT_PROPERTY(table, "Height scale", core::TerrainComponent, terrain, heightScale);
//    EDIT_PROPERTY(table, "Layer texture array", core::TerrainComponent, terrain, layers);
//    EDIT_PROPERTY(table, "Splat texture", core::TerrainComponent, terrain, splat);
//    EDIT_PROPERTY(table, "Size", core::TerrainComponent, terrain, size, 0.001f, 10000.f);
//}
//
//void GameObjectEditor::Populate(core::GameObject& go, const core::FoliageComponent& comp) const {
//    PropertyTable table("Foliage", gui_, images_);
//
//    EDIT_PROPERTY(table, "Layers", core::FoliageComponent, comp, layerCount, 0, core::FoliageComponent::MAX_LAYERS);
//
//    int _id{};
//    for (uint32_t i = 0; i < comp.layerCount; ++i) {
//        ImScope::Id id{ _id };
//
//        ImGui::Separator();
//
//        table.ConstProperty("Layer", std::to_string(i));
//
//        const auto& layer{ comp.layers[i] };
//        {
//            auto count{ layer.count };
//            if (table.EditProperty("Count", count)) {
//                go.Patch<core::FoliageComponent>([&](auto& c) { c.layers[i].count = count; });
//            }
//        }
//
//        {
//            auto shadows{ layer.shadows };
//            if (table.EditProperty("Shadows", shadows)) {
//                go.Patch<core::FoliageComponent>([&](auto& c) { c.layers[i].shadows = shadows; });
//            }
//        }
//
//        table.BeginProperty("Foliage");
//        if (ImGui::BeginCombo("##foliage", layer.foliage ? layer.foliage->name.c_str() : "<none>")) {
//            for (const auto& foliage : FoliageService::Foliages()) {
//                bool selected{ layer.foliage == foliage };
//                if (ImGui::Selectable(foliage->name.c_str(), &selected)) {
//                    go.Patch<core::FoliageComponent>([&](auto& c) { c.layers[i].foliage = foliage; });
//                }
//            }
//
//            ImGui::EndCombo();
//        }
//        table.EndProperty();
//
//        table.ConstProperty("Real count", std::to_string(comp.realCount[i]));
//    }
//}

} // namespace ugine::ed