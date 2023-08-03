#pragma once

#include <ugine/engine/gfx/Animation.h>
#include <ugine/engine/gfx/ImGui.h>
#include <ugine/engine/gfx/Material.h>
#include <ugine/engine/gfx/Model.h>
#include <ugine/engine/gfx/Texture.h>
#include <ugine/engine/script/LuaScript.h>
#include <ugine/engine/world/World.h>

#include "ImGuiEx.h"

#include <optional>
#include <variant>

namespace ugine::ed {

class DragAndDrop {
public:
    explicit DragAndDrop(ugine::ResourceManager& resources)
        : resources_{ resources } {}

    template <typename T> void BeginDrag(T value, StringView name) {
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
            Reset();
            value_ = value;
            ImGui::SetDragDropPayload(Payload<T>(), this, sizeof(this));
            ImGuiEx::Text(name);
            ImGui::EndDragDropSource();
        }
    }

    template <typename F> void BeginDrag(F f) {
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
            Reset();
            auto [value, name] = f();
            value_ = value;
            ImGui::SetDragDropPayload(Payload<decltype(value)>(), this, sizeof(this));
            ImGuiEx::Text(name);
            ImGui::EndDragDropSource();
        }
    }

    template <typename T> bool Drop(T& value) {
        bool res{};
        if (ImGui::BeginDragDropTarget()) {
            res = Accept<T>(value);
            ImGui::EndDragDropTarget();
        }
        return res;
    }

    template <typename T> bool DropResource(ugine::ResourceHandle<T>& value) {
        bool res{};
        if (ImGui::BeginDragDropTarget()) {
            auto ref{ std::get_if<ResourceReference>(&value_) };
            if (ref && ref->type == T::TYPE) {
                const ImGuiPayload* payload{ ImGui::AcceptDragDropPayload(Payload<ResourceReference>()) };
                if (payload) {
                    value = resources_.Get<T>(ref->id);
                    Reset();
                    res = true;
                }
            }
            ImGui::EndDragDropTarget();
        }
        return res;
    }

    bool DropResource(ugine::ResourceID& id, const ugine::ResourceType& type) {
        bool res{};
        if (ImGui::BeginDragDropTarget()) {
            auto ref{ std::get_if<ResourceReference>(&value_) };
            if (ref && ref->type == type) {
                const ImGuiPayload* payload{ ImGui::AcceptDragDropPayload(Payload<ResourceReference>()) };
                if (payload) {
                    id = ref->id;
                    Reset();
                    res = true;
                }
            }
            ImGui::EndDragDropTarget();
        }
        return res;
    }

    template <typename F> bool DropResourceMulti(F f) {
        bool res{};
        if (ImGui::BeginDragDropTarget()) {
            if (auto ref = std::get_if<ResourceReferences>(&value_)) {
                if (auto payload = ImGui::AcceptDragDropPayload(Payload<ResourceReferences>())) {
                    for (const auto& resource : *ref) {
                        f(resource.id, resource.type);
                    }
                    Reset();
                    res = true;
                }
            } else if (auto ref = std::get_if<ResourceReference>(&value_)) {
                if (auto payload = ImGui::AcceptDragDropPayload(Payload<ResourceReference>())) {
                    f(ref->id, ref->type);
                    Reset();
                    res = true;
                }
            }
            ImGui::EndDragDropTarget();
        }
        return res;
    }

private:
    template <typename T> bool Accept(T& value) {
        const ImGuiPayload* payload{ ImGui::AcceptDragDropPayload(Payload<T>()) };
        if (payload) {
            value = std::get<T>(value_);
            Reset();
            return true;
        }
        return false;
    }

    template <typename T> const char* Payload() const;

    template <> const char* Payload<ugine::GameObject>() const { return "ugine::gameobject"; }
    template <> const char* Payload<ResourceReference>() const { return "ugine::resource"; }
    template <> const char* Payload<ResourceReferences>() const { return "ugine::resources"; }

    void Reset() { value_ = {}; }

    ugine::ResourceManager& resources_;

    std::variant<ugine::GameObject, ResourceReference, ResourceReferences> value_;
};

} // namespace ugine::ed