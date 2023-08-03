#include "WorldEditor.h"
#include "Actions.h"
#include "WorldTool.h"

#include "../EditorContext.h"
#include "../widgets/ImGuiScope.h"

#include <ugine/engine/gfx/Component.h>
#include <ugine/engine/gfx/ImGui.h>
#include <ugine/engine/script/Component.h>
#include <ugine/engine/world/Component.h>
#include <ugine/engine/world/GameObject.h>
#include <ugine/engine/world/WorldManager.h>
#include <ugine/engine/world/WorldSerializer.h>

#include <ugine/Log.h>
#include <ugine/Profile.h>

#include <glm/gtc/type_ptr.hpp>

namespace ugine::ed {

WorldEditor::WorldEditor(EditorContext& context)
    : EditorTool{ context }
    , saveWindow_{ context } {
    auto& view{ context_.MainMenu().Get("View") };

    view.AddAction([this] { return ImGui::Checkbox("Scene editor", &visible_); });

    auto& file{ context_.MainMenu().Get("File") };
    file.AddAction(WORLD_RESOURCE_ICON " New world", [this] { NewWorld(); });
    file.AddAction(WORLD_RESOURCE_ICON " Save world", [this] { SaveWorld(); });
    file.AddSeparator();

    context_.Events().Connect<OpenProjectEvent, &WorldEditor::OnOpenProject>(this);

    context_.Events().Connect<OpenWorldEvent, &WorldEditor::OnOpenWorld>(this);
    context_.Events().Connect<SaveWorldEvent, &WorldEditor::OnSaveWorld>(this);

    context_.Events().Connect<OpenResourceEvent<WorldDescriptor>, &WorldEditor::OnOpenWorldResource>(this);
    context_.Events().Connect<OpenTypelessResourceEvent, &WorldEditor::OnOpenTypelessResource>(this);
}

void WorldEditor::OnOpenProject(const OpenProjectEvent& event) {
    NewWorld();
}

void WorldEditor::NewWorld() {
    worldPath_ = std::nullopt;

    try {
        if (context_.ActiveWorld()) {
            context_.DeselectGO();
            context_.Engine().GetWorldManager().DestroyWorld(context_.ActiveWorld());
        }

        auto newWorld{ context_.Engine().GetWorldManager().CreateWorld() };
        newWorld->SetName("GameWorld");
        NewWorldTemplate(newWorld);
        newWorld->SetRendering(true);

        context_.Events().ChangeWorld(newWorld);
    } catch (const std::exception& ex) {
        context_.Events().Error(ex.what());
    }
}

void WorldEditor::OnOpenWorldResource(const OpenResourceEvent<WorldDescriptor>& event) {
    OpenWorld(event.resource);
}

void WorldEditor::OnOpenTypelessResource(const OpenTypelessResourceEvent& event) {
    if (event.resource.type == WorldDescriptor::TYPE) {
        OpenWorld(context_.Engine().GetResources().Get<WorldDescriptor>(event.resource.id));
    }
}

void WorldEditor::OnOpenWorld(const OpenWorldEvent& event) {
    OpenWorld(event.world, event.clear);
}

void WorldEditor::OnSaveWorld(const SaveWorldEvent& event) {
    if (worldPath_) {
        WorldSerializer serializer{};
        serializer.Serialize(*context_.ActiveWorld(), *worldPath_);

        UGINE_INFO("World saved: {}", worldPath_->String());
    } else {
        SaveWorld();
    }
}

void WorldEditor::OpenWorld(ResourceHandle<WorldDescriptor> handle, bool clear) {
    const Path path{ context_.Engine().GetResources().ResourcePath(handle->Id()) };
    if (path.Empty()) {
        context_.Engine().GetResources().Delete<WorldDescriptor>(handle->Id());
        context_.Events().Error("Error opening world");
        return;
    }

    try {
        context_.DeselectGO();

        if (clear) {
            worldPath_ = path;

            context_.ActiveWorld()->Clear();
        }

        WorldSerializer des{};
        des.Deserialize(*context_.ActiveWorld(), context_.Engine(), path);
    } catch (const std::exception& ex) {
        context_.Events().Error(ex.what());
    }

    context_.Events().ChangeWorld(context_.ActiveWorld());
}

void WorldEditor::SaveWorld() {
    saveWindow_.SetSaveAction("World", [this](const Path& targetPath, StringView fileName) {
        auto [resource, path] = context_.CreateResource<WorldDescriptor>(targetPath, fileName);

        WorldSerializer serializer{};
        serializer.Serialize(*context_.ActiveWorld(), path);
        UGINE_INFO("World saved: {}", path.String());

        worldPath_ = path;
    });

    context_.Events().ShowModal(&saveWindow_);
}

void WorldEditor::PopulateGameObject(GameObject& go, int& id) {
    if (go.Flags() & TagComponent::Flags::Editor) {
        return;
    }

    PROFILE_EVENT_NC("GameObject", COLOR_EDITOR_PROFILE);

    ImScope::Id sId{ &id };

    ImScope::Color color(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImScope::Color colorHover(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
    ImScope::Color colorActive(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));

    const uint32_t goFlags{ go.Has<GuiFlags>() ? go.Component<GuiFlags>().flags : 0 };

    int flags{ ImGuiTreeNodeFlags_OpenOnArrow };
    flags |= ImGuiTreeNodeFlags_FramePadding;
    flags |= ImGuiTreeNodeFlags_DefaultOpen;
    flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
    flags |= (go.ChildrenCount() > 0 ? 0 : ImGuiTreeNodeFlags_Leaf);

    if ((goFlags & GuiFlags::Selected) != 0) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    auto popStyle{ false };
    if (!go.IsEnabled()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
        popStyle = true;
    }

    auto pos{ ImGui::GetCursorPos() };
    ImGui::SetCursorPosX(8);
    if (ImGui::Button(go.IsEnabled() ? ICON_FA_EYE : ICON_FA_EYE_OFF)) {
        go.SetEnabled(!go.IsEnabled());
    }

    ImGui::SetCursorPos(ImVec2{ pos.x + 32, pos.y });

    PopulateIcons(go);
    auto open{ ImGui::TreeNodeEx(go.Name().c_str(), flags, go.Name().c_str()) };
    context_.DragAndDrop().BeginDrag(go, "Game object");

    GameObject drop;
    if (context_.DragAndDrop().Drop<GameObject>(drop)) {
        if (drop != go) {
            bool fail{};
            for (auto p = go.Parent(); p; p = p.Parent()) {
                if (p == drop) {
                    fail = true;
                    break;
                }
            }

            if (!fail) {
                if (drop.Parent()) {
                    drop.Parent().RemoveChild(drop);
                }

                go.AddChild(drop);
            }
        }
    }

    ResourceID resID{};
    ResourceType resType{};
    //if (context_.DragAndDrop().DropResource(resID, resType)) {
    //    AddResource(context_, go, resID, resType);
    //}

    context_.DragAndDrop().DropResourceMulti([&](const ResourceID& resID, const ResourceType& resType) { AddResource(context_, go, resID, resType); });

    if (ImGui::IsItemClicked()) {
        context_.SelectGO(go);
    }

    const auto rightEdge{ ImGui::GetCursorPosX() + ImGui::GetColumnWidth() };

    if (open) {
        for (auto child{ go.FirstChild() }; child.IsValid(); child = child.NextSibling()) {
            PopulateGameObject(child, id);
        }

        ImGui::TreePop();
    }

    if (popStyle) {
        ImGui::PopStyleColor();
    }
}

void WorldEditor::PopulateIcons(const GameObject& go) {
    if (go.Has<MeshComponent>()) {
        ImGui::TextUnformatted(ICON_FA_CUBE " ");
        ImGui::SameLine();
    }

    if (go.Has<AnimationControllerComponent>()) {
        ImGui::TextUnformatted(ICON_FA_FILM " ");
        ImGui::SameLine();
    }

    //if (go.Has<ParticleComponent>()) {
    //    str << ICON_FA_ATOM << " ";
    //}

    //if (go.Has<TerrainComponent>()) {
    //    str << ICON_FA_MOUNTAIN << " ";
    //}

    //if (go.Has<FoliageComponent>()) {
    //    str << ICON_FA_TREE << " ";
    //}

    if (go.Has<CameraComponent>()) {
        ImGui::TextUnformatted(ICON_FA_CAMERA " ");
        ImGui::SameLine();
    }

    if (go.Has<LightComponent>()) {
        ImGui::TextUnformatted(ICON_FA_LIGHTBULB " ");
        ImGui::SameLine();
    }

    if (go.Has<LuaScriptComponent>()) {
        ImGui::TextUnformatted(ICON_FA_CLIPBOARD " ");
        ImGui::SameLine();
    }

    if (go.Has<SkyComponent>()) {
        ImGui::TextUnformatted(ICON_FA_CLOUD " ");
        ImGui::SameLine();
    }
}

void WorldEditor::NewWorldTemplate(World* world) const {
    // TODO: New World Template
    {
        auto dirLight{ world->CreateObject("Direction light") };
        dirLight.CreateComponent<LightComponent>(LightComponent{
            .type = LightComponent::Type::Directional,
            .generatesShadows = true,
        });
        dirLight.SetGlobalTransformation(Transformation{ glm::vec3{}, LookAt(glm::vec3{}, glm::vec3{ -1, 1, 1 }), glm::vec3{ 1.0f } });
    }

    {
        auto camera{ world->CreateObject("Camera") };
        camera.CreateComponent<CameraComponent>(CameraComponent{
            .isMain = true,
        });
        camera.SetGlobalTransformation(Transformation{ glm::vec3{}, LookAt(glm::vec3{}, glm::vec3{ 0, 0, -1 }), glm::vec3{ 1.0f } });
    }
}

void WorldEditor::RenderToolbar() {
    int id{};
    {
        ImScope::Id _id{ &id };
        if (ImGui::Button(ICON_FA_PLUS)) {
            auto go{ context_.ActiveWorld()->CreateObject("Empty") };
            if (context_.SelectedGO()) {
                context_.SelectedGO().AddChild(go);
            }
        }
    }

    if (context_.SelectedGO().IsValid()) {
        ImGui::SameLine();

        {
            ImScope::Id _id{ &id };
            if (ImGui::Button(ICON_FA_MINUS)) {
                auto toDelete{ context_.SelectedGO() };
                context_.DeselectGO();
                GameObject::Destroy(toDelete);
            }
        }
        ImGui::SameLine();

        {
            ImScope::Id _id{ &id };
            if (ImGui::Button(ICON_FA_CONTENT_COPY)) {
                DuplicateGO(context_, context_.SelectedGO());
            }
        }
    }
}

void WorldEditor::RenderSceneList() {
    ImScope::Color style{ ImGuiCol_FrameBg, ImVec4{ 0.8f, 0.8f, 0.8f, 1.0f } };

    PopulateGameObjects(*context_.ActiveWorld());
}

void WorldEditor::PopulateGameObjects(World& scene) {
    PROFILE_EVENT_NC("ObjectTree", COLOR_EDITOR_PROFILE);

    bool selected{ true };
    for (auto [ent, gui] : scene.Registry().view<GuiFlags>().each()) {
        auto flags{ gui.flags };
        if (flags & GuiFlags::Selected) {
            selected = false;
            break;
        }
    }

    if (ImGui::Selectable(ICON_FA_SITEMAP "  Scene", &selected)) {
        context_.DeselectGO();
    }

    GameObject go{};
    if (context_.DragAndDrop().Drop<GameObject>(go)) {
        if (go.Parent()) {
            go.Parent().RemoveChild(go);
        }
    }

    int id{ 0 };
    scene.Registry().group<TagComponent>({}, entt::exclude<ParentComponent>).each([&](auto ent, const TagComponent&) {
        auto go{ scene.Get(ent) };
        PopulateGameObject(go, id);
    });
}

void WorldEditor::Render() {
    PROFILE_EVENT_NC("SceneEditor", COLOR_EDITOR_PROFILE);

    if (ImGui::Begin(ICON_FA_FORMAT_LIST_BULLETED " Scene hierarchy##SceneEditor")) {
        if (context_.ActiveWorld()) {
            RenderToolbar();

            ImGui::Separator();

            RenderSceneList();
        }
    }

    if (ImGui::IsWindowFocused()) {
        WorldShortcuts(context_);
    }

    ImGui::End();
}

} // namespace ugine::ed