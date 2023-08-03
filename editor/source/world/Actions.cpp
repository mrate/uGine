#include "Actions.h"

#include "../EditorContext.h"

#include <ugine/engine/gfx/Component.h>
#include <ugine/engine/script/Component.h>

#include <imgui.h>

namespace ugine::ed {

bool WorldShortcuts(EditorContext& context) {
    if (ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
        auto selected{ context.SelectedGO() };

        if (selected) {
            context.DeselectGO();
            context.ActiveWorld()->DestroyObject(selected);
        }

        return true;
    }

    if (ImGui::GetIO().KeyCtrl) {
        if (ImGui::IsKeyPressed(ImGuiKey_S, false)) {
            context.Events().SaveWorld();

            return true;
        } else if (ImGui::IsKeyPressed(ImGuiKey_D, false)) {
            auto selected{ context.SelectedGO() };
            if (selected) {
                DuplicateGO(context, selected);
            }

            return true;
        }
    }

    return false;
}

void DuplicateGO(EditorContext& context, GameObject go) {
    context.SelectGO(go.Clone());
    context.SelectedGO().SetName(go.Name() + " Copy");
}

void AddResource(EditorContext& context, GameObject& go, const ResourceID& id, const ResourceType& type) {
    if (type == Model::TYPE) {
        if (!go.Has<MeshComponent>()) {
            go.CreateComponent<MeshComponent>(context.Engine().GetResources().Get<Model>(id));
        }
    } else if (type == Animation::TYPE) {
        if (!go.Has<AnimationControllerComponent>()) {
            go.CreateComponent<AnimationControllerComponent>(context.Engine().GetResources().Get<Animation>(id));
        }
    } else if (type == LuaScript::TYPE) {
        if (!go.Has<LuaScriptComponent>()) {
            go.CreateComponent<LuaScriptComponent>(context.Engine().GetResources().Get<LuaScript>(id));
        }
    }
}

} // namespace ugine::ed