#pragma once

#include "ImGuiEx.h"
#include "ImGuiScope.h"
#include "ResourceSelectWidget.h"

#include "../EditorContext.h"

#include <ugine/Color.h>
#include <ugine/Log.h>
#include <ugine/engine/world/Transformation.h>

#include <imgui.h>

#include <glm/glm.hpp>

#include <map>

namespace ugine::ed {

struct PropertyTable {
    PropertyTable(ugine::StringView id, EditorContext* context = nullptr, int columns = 2, bool setup = true, bool stretchProperty = true)
        : context_{ context }
        , columns_{ columns }
        , stretchPropery_{ stretchProperty } {
        const auto flags{ ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit };
        const auto val{ ImGui::BeginTable("##Property_table", columns, flags) };
        if (val == 0) {
            UGINE_ASSERT(false && "Probably some ImGui::Begin not in if statement");
            throw std::runtime_error{ "Invalid state" };
        }

        if (setup) {
            ImGui::TableSetupColumn("##name", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("##property", ImGuiTableColumnFlags_WidthStretch);
        }

        ImGui::PushID(id.Data());
    }

    ~PropertyTable() {
        ImGui::PopID();
        ImGui::EndTable();
    }

    // Getter / Setter.
    template <typename T> bool GuiWidget(T& object, float (T::*getter)() const, void (T::*setter)(float), float min, float max) {
        auto value{ (object.*(getter))() };
        if (ImGui::SliderFloat("", &value, min, max)) {
            (object.*(setter))(value);
            return true;
        }
        return false;
    }

    template <typename T> bool GuiWidget(T& object, int (T::*getter)() const, void (T::*setter)(int), int min, int max) {
        auto value{ (object.*(getter))() };
        if (ImGui::SliderInt("", &value, min, max)) {
            (object.*(setter))(value);
            return true;
        }
        return false;
    }

    template <typename T> bool GuiWidget(T& object, const ugine::ColorRGBA& (T::*getter)() const, void (T::*setter)(const ugine::ColorRGBA&)) {
        auto value{ (object.*(getter))() };
        if (ColorGui("", value)) {
            (object.*(setter))(value);
            return true;
        }
        return false;
    }

    template <typename T> bool GuiWidget(T& object, bool (T::*getter)() const, void (T::*setter)(bool)) {
        auto value{ (object.*(getter))() };
        if (ImGui::Checkbox("", &value)) {
            (object.*(setter))(value);
            return true;
        }
        return false;
    }

    template <typename T>
    bool GuiWidget(T& object, const glm::vec2& (T::*getter)() const, void (T::*setter)(const glm::vec2&), float min, float max, float speed = 1.0f) {
        auto& value{ (object.*(getter))() };
        float values[2] = { value.x, value.y };
        if (ImGui::DragFloat2("", values, speed, min, max)) {
            (object.*(setter))(glm::vec2(values[0], values[1]));
            return true;
        }
        return false;
    }

    template <typename T>
    bool GuiWidget(
        T& object, const glm::vec3& (T::*getter)() const, void (T::*setter)(const glm::vec3&), bool asColor, float min, float max, float speed = 1.0f) {
        auto value{ (object.*(getter))() };
        if (ImGuiEx::Vec3Editor("", value, asColor, min, max, speed)) {
            (object.*(setter))(value);
            return true;
        }
        return false;
    }

    template <typename T>
    bool GuiWidget(
        T& object, const glm::vec4& (T::*getter)() const, void (T::*setter)(const glm::vec4&), bool asColor, float min, float max, float speed = 1.0f) {
        const auto& value{ (object.*(getter))() };
        float values[4] = { value.x, value.y, value.z, value.w };
        if (ImGuiEx::Vec4Editor("", values, asColor, min, max, speed)) {
            (object.*(setter))(glm::vec4(values[0], values[1], values[2], values[3]));
            return true;
        }
        return false;
    }

    template <typename T> bool GuiWidget(T& object, const std::string& (T::*getter)() const, void (T::*setter)(const std::string&)) {
        // TODO: String.
        char buffer[64];
        snprintf(buffer, 64, (object.*(getter))().c_str());
        if (ImGui::InputText("", buffer, 64)) {
            (object.*(setter))(buffer);
            return true;
        }
        return false;
    }

    template <typename T> bool GuiWidget(T& object, const ugine::String& (T::*getter)() const, void (T::*setter)(const ugine::String&)) {
        // TODO: String.
        char buffer[64];
        snprintf(buffer, 64, (object.*(getter))().Data());
        if (ImGui::InputText("", buffer, 64)) {
            (object.*(setter))(buffer);
            return true;
        }
        return false;
    }

    // By value.
    bool GuiWidget(bool& value) { return ImGui::Checkbox("", &value); }
    bool GuiWidget(float& value) { return ImGui::InputFloat("", &value); }
    bool GuiWidget(float& value, float min, float max) { return ImGui::SliderFloat("", &value, min, max); }
    bool GuiWidget(int& value) { return ImGui::InputInt("", &value); }
    bool GuiWidget(int& value, int min, int max) { return ImGui::SliderInt("", &value, min, max); }

    bool GuiWidget(uint32_t& value) {
        auto i{ static_cast<int>(value) };
        if (ImGui::InputInt("", &i)) {
            value = i;
            return true;
        }
        return false;
    }

    bool GuiWidget(uint32_t& value, uint32_t min, uint32_t max) {
        auto i{ static_cast<int>(value) };
        if (ImGui::InputInt("", &i, min, max)) {
            value = i;
            return true;
        }
        return false;
    }

    bool GuiWidget(ugine::ColorRGB& value) { return ImGuiEx::ColorEditor("", value); }
    bool GuiWidget(ugine::ColorRGBA& value) { return ImGuiEx::ColorEditor("", value); }
    bool GuiWidget(glm::vec2& value, float min = 0.0f, float max = 0.0f, float speed = 1.0f) { return ImGuiEx::Vec2Editor("", value, min, max, speed); }
    bool GuiWidget(glm::vec3& value, bool asColor = false, float min = 0.0f, float max = 0.0f, float speed = 1.0f) {
        return ImGuiEx::Vec3Editor("", value, asColor, min, max, speed);
    }
    bool GuiWidget(glm::vec4& value, bool asColor = false, float min = 0.0f, float max = 0.0f, float speed = 1.0f) {
        return ImGuiEx::Vec4Editor("", value, asColor, min, max, speed);
    }
    bool GuiWidget(glm::ivec2& value, int min = 0.0f, int max = 0.0f) {
        int val[2] = { value.x, value.y };
        if (ImGui::SliderInt2("", val, min, max)) {
            value = glm::ivec2(val[0], val[1]);
            return true;
        }
        return false;
    }

    bool GuiWidget(glm::ivec3& value, int min = 0.0f, int max = 0.0f, int speed = 1.0f) {
        int val[3] = { value.x, value.y, value.z };
        if (ImGui::SliderInt3("", val, min, max)) {
            value = glm::ivec3(val[0], val[1], val[2]);
            return true;
        }
        return false;
    }

    bool GuiWidget(glm::ivec4& value, int min = 0.0f, int max = 0.0f, int speed = 1.0f) {
        int val[4] = { value.x, value.y, value.z, value.w };
        if (ImGui::SliderInt4("", val, min, max)) {
            value = glm::ivec4(val[0], val[1], val[2], val[3]);
            return true;
        }
        return false;
    }

    bool GuiWidget(std::string& str) {
        // TODO: String.
        char buffer[64];
        snprintf(buffer, 64, str.c_str());
        if (ImGui::InputText("", buffer, 64)) {
            str = buffer;
            return true;
        }
        return false;
    }

    bool GuiWidget(ugine::String& str) {
        // TODO: String.
        char buffer[64];
        snprintf(buffer, 64, str.Data());
        if (ImGui::InputText("", buffer, 64)) {
            str = buffer;
            return true;
        }
        return false;
    }

    template <typename T> bool GuiWidget(ugine::ResourceHandle<T>& handle) {
        if (context_) {
            return ImGuiEx::SelectResource(*context_, handle);
        } else {
            UGINE_ASSERT(false);
            ImGui::TextUnformatted("<missing editor context>");
            return false;
        }
    }

    template <typename... Args> bool EditProperty(ugine::StringView name, Args&&... args) {
        ImScope::Id id{ &id_ };

        BeginProperty(name.Data());
        auto result{ GuiWidget(std::forward<Args>(args)...) };
        EndProperty();

        return result;
    }

    template <typename T> bool EditPropertyCombo(ugine::StringView name, T& value, const std::map<const char*, T>& values) {
        ImScope::Id id{ &id_ };

        const char* label = "<invalid>";
        for (auto [k, v] : values) {
            if (v == value) {
                label = k;
                break;
            }
        }

        bool result{};

        BeginProperty(name.Data());
        if (ImGui::BeginCombo("", label)) {
            for (auto [k, v] : values) {
                bool selected{ v == value };
                if (ImGui::Selectable(k, selected)) {
                    value = v;
                    result = true;
                }
            }
            ImGui::EndCombo();
        }
        EndProperty();

        return result;
    }

    void ConstPropertyUnformatted(ugine::StringView name, ugine::StringView value) {
        //ImScope::Color style{ ImGuiCol_Text, ImVec4{ 0.4f, 0.4f, 0.4f, 1.0f } };
        BeginProperty(name);
        ImGui::TextUnformatted(value.Data());
        EndProperty();
    }

    template <typename... Args> void ConstProperty(ugine::StringView name, ugine::StringView fmt, Args... args) {
        BeginProperty(name);
        ImGui::Text(fmt.Data(), args...);
        EndProperty();
    }

    void ConstProperty(ugine::StringView name, ugine::StringView text) {
        BeginProperty(name);
        ImGui::TextUnformatted(text.Data());
        EndProperty();
    }

    void ConstProperty(ugine::StringView name, float value) {
        BeginProperty(name);
        ImGui::Text("%f", value);
        EndProperty();
    }

    // TODO: [PATH]
    void ConstProperty(ugine::StringView name, const std::filesystem::path& value) {
        BeginProperty(name);
        ImGui::TextUnformatted(value.string().c_str());
        EndProperty();
    }

    void ConstProperty(ugine::StringView name, const Path& value) {
        BeginProperty(name);
        ImGui::TextUnformatted(value.Data());
        EndProperty();
    }

    void ConstProperty(ugine::StringView name, const glm::vec2& value) {
        BeginProperty(name);
        ImGui::Text("[%f, %f]", value.x, value.y);
        EndProperty();
    }

    void ConstProperty(ugine::StringView name, const glm::vec3& value) {
        BeginProperty(name);
        ImGui::Text("[%f, %f, %f]", value.x, value.y, value.z);
        EndProperty();
    }

    void ConstProperty(ugine::StringView name, const glm::vec4& value) {
        BeginProperty(name);
        ImGui::Text("[%f, %f, %f, %f]", value.x, value.y, value.z, value.w);
        EndProperty();
    }

    void ConstProperty(ugine::StringView name, int value) {
        BeginProperty(name);
        ImGui::Text("%d", value);
        EndProperty();
    }

    void ConstProperty(ugine::StringView name, const glm::ivec2& value) {
        BeginProperty(name);
        ImGui::Text("[%d, %d]", value.x, value.y);
        EndProperty();
    }

    void ConstProperty(ugine::StringView name, const glm::ivec3& value) {
        BeginProperty(name);
        ImGui::Text("[%d, %d, %d]", value.x, value.y, value.z);
        EndProperty();
    }

    void ConstProperty(ugine::StringView name, const glm::ivec4& value) {
        BeginProperty(name);
        ImGui::Text("[%d, %d, %d, %d]", value.x, value.y, value.z, value.w);
        EndProperty();
    }

    void ConstProperty(ugine::StringView name, const ugine::ColorRGBA& value) {
        BeginProperty(name);
        ImGui::Text("[%d, %d, %d, %d]", 255 * value.r, 255 * value.g, 255 * value.b, 255 * value.a);
        EndProperty();
    }

    template <typename T> void ConstProperty(ugine::StringView name, ugine::ResourceHandle<T> handle) {
        BeginProperty(name);
        if (handle) {
            if (context_) {
                const auto& name{ context_->Engine().GetResources().ResourceName(handle->Id()) };
                ImGui::TextUnformatted(name.c_str());
            } else {
                UGINE_ASSERT(false);
                ImGui::TextUnformatted("<missing editor context>");
            }
        } else {
            ImGui::TextUnformatted("<none>");
        }
        EndProperty();
    }

    template <typename... Args> void BeginProperty(ugine::StringView name, Args... args) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text(name.Data(), args...);

        ImGui::TableSetColumnIndex(1);
        if (stretchPropery_) {
            ImGui::PushItemWidth(-1);
        }
    }

    void EndProperty() {
        if (stretchPropery_) {
            ImGui::PopItemWidth();
        }
    }

private:
    int id_{};
    EditorContext* context_{};
    int columns_{ 2 };
    bool stretchPropery_{};
};

} // namespace ugine::ed