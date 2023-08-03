#include "ImGuiEx.h"

#include "../platform/FileDialog.h"
#include "PropertyTable.h"

#include <ugine/engine/engine/CVars.h>
#include <ugine/engine/gfx/Uniform.h>
#include <ugine/engine/math/Math.h>

#include <ugine/engine/math/Math.h>
#include <ugine/engine/world/Transformation.h>

#include <glm/gtc/type_ptr.hpp>

#include <imgui_internal.h>

using namespace ImGui;

using namespace ugine;
using namespace ugine::ed;

namespace ImGuiEx {

const ImVec4 COLOR_X = { 0.8f, 0.3f, 0.3f, 1.0f };
const ImVec4 COLOR_Y = { 0.3f, 0.8f, 0.3f, 1.0f };
const ImVec4 COLOR_Z = { 0.3f, 0.3f, 0.8f, 1.0f };
const ImVec4 COLOR_W = { 0.8f, 0.8f, 0.5f, 1.0f };

constexpr float HOVER{ 1.5f };

const ImVec4 COLOR_X_HOVER = { COLOR_X.x * HOVER, COLOR_X.y* HOVER, COLOR_X.z* HOVER, 1.0f };
const ImVec4 COLOR_Y_HOVER = { COLOR_Y.x * HOVER, COLOR_Y.y* HOVER, COLOR_Y.z* HOVER, 1.0f };
const ImVec4 COLOR_Z_HOVER = { COLOR_Z.x * HOVER, COLOR_Z.y* HOVER, COLOR_Z.z* HOVER, 1.0f };
const ImVec4 COLOR_W_HOVER = { COLOR_W.x * HOVER, COLOR_W.y* HOVER, COLOR_W.z* HOVER, 1.0f };

constexpr float ACTIVE{ 0.5f };

const ImVec4 COLOR_X_ACTIVE = { COLOR_X.x * ACTIVE, COLOR_X.y* ACTIVE, COLOR_X.z* ACTIVE, 1.0f };
const ImVec4 COLOR_Y_ACTIVE = { COLOR_Y.x * ACTIVE, COLOR_Y.y* ACTIVE, COLOR_Y.z* ACTIVE, 1.0f };
const ImVec4 COLOR_Z_ACTIVE = { COLOR_Z.x * ACTIVE, COLOR_Z.y* ACTIVE, COLOR_Z.z* ACTIVE, 1.0f };
const ImVec4 COLOR_W_ACTIVE = { COLOR_W.x * ACTIVE, COLOR_W.y* ACTIVE, COLOR_W.z* ACTIVE, 1.0f };

void MatrixToFloatArray(const glm::mat4& mat, float* array) {
    memcpy(array, glm::value_ptr(mat), sizeof(float) * 16);
}

void FloatArrayToMatrix(const float* array, glm::mat4& mat) {
    memcpy(glm::value_ptr(mat), array, sizeof(float) * 16);
}

bool QuatEditor(const char* title, glm::fquat& quat, float min, float max, float speed) {
    //glm::vec4 v{ quat.x, quat.y, quat.z, quat.w };
    //if (Vec4Gui(title, v, min, max, speed)) {
    //    quat = glm::normalize(glm::fquat{ v.w, v.x, v.y, v.z });
    //    return true;
    //}
    auto vec{ ugine::QuatToEuler(quat) };
    if (Vec3Editor(title, vec)) {
        //vec.x = std::min(std::max(vec.x, -89.9f), 89.9f);
        //vec = glm::vec3(fmod(vec.x, 90.0f), fmod(vec.y, 180.0f), vec.z);
        quat = glm::fquat(glm::radians(vec));
        return true;
    }

    return false;
}

bool Vec4Editor(const char* title, glm::vec4& vec, bool asColor, float min, float max, float speed) {
    ImScope::Id id(title);

    if (asColor) {
        float f[4] = { vec.x, vec.y, vec.z, vec.w };
        if (ImGui::ColorEdit4(title, f)) {
            vec.x = f[0];
            vec.y = f[1];
            vec.z = f[2];
            vec.w = f[3];
            return true;
        }
        return false;
    }

    bool changed{ false };

    const auto size{ ImGui::GetContentRegionAvail() };
    ImScope::ItemWidth itemWidth((size.x - 4) / 4);
    ImScope::StyleVar noSpacing(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

    {
        ImScope::Color color(ImGuiCol_Border, COLOR_X);
        changed |= ImGui::DragFloat("##x", &vec.x, speed, min, max);
    }

    ImGui::SameLine();

    {
        ImScope::Color border(ImGuiCol_Border, COLOR_Y);
        changed |= ImGui::DragFloat("##y", &vec.y);
    }

    ImGui::SameLine();

    {
        ImScope::Color border(ImGuiCol_Border, COLOR_Z);
        changed |= ImGui::DragFloat("##z", &vec.z);
    }

    ImGui::SameLine();

    {
        ImScope::Color border(ImGuiCol_Border, COLOR_W);
        changed |= ImGui::DragFloat("##w", &vec.w);
    }

    return changed;
}

bool Vec3Editor(const char* title, glm::vec3& vec, bool asColor, float min, float max, float speed) {
    ImScope::Id id(title);

    if (asColor) {
        float f[3] = { vec.x, vec.y, vec.z };
        if (ImGui::ColorEdit3(title, f)) {
            vec.x = f[0];
            vec.y = f[1];
            vec.z = f[2];
            return true;
        }
        return false;
    }

    bool changed{ false };

    const auto size{ ImGui::GetContentRegionAvail() };
    ImScope::ItemWidth itemWidth((size.x - 4) / 3);
    ImScope::StyleVar noSpacing(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

    {
        ImScope::Color border(ImGuiCol_Border, COLOR_X);
        changed |= ImGui::DragFloat("##x", &vec.x, speed, min, max);
    }

    ImGui::SameLine();

    {
        ImScope::Color border(ImGuiCol_Border, COLOR_Y);
        changed |= ImGui::DragFloat("##y", &vec.y);
    }

    ImGui::SameLine();

    {
        ImScope::Color border(ImGuiCol_Border, COLOR_Z);
        changed |= ImGui::DragFloat("##z", &vec.z);
    }

    return changed;
}

bool Vec2Editor(const char* title, glm::vec2& vec, float min, float max, float speed) {
    ImScope::Id id(title);
    bool changed{ false };

    const auto size{ ImGui::GetContentRegionAvail() };
    ImScope::ItemWidth itemWidth((size.x - 4) / 3);
    ImScope::StyleVar noSpacing(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

    {
        ImScope::Color border(ImGuiCol_Border, COLOR_X);
        changed |= ImGui::DragFloat("##x", &vec.x, speed, min, max);
    }

    ImGui::SameLine();

    {
        ImScope::Color border(ImGuiCol_Border, COLOR_Y);
        changed |= ImGui::DragFloat("##y", &vec.y);
    }

    return changed;
}

bool ColorEditor(const char* title, ugine::ColorRGBA& color) {
    float f[4] = { color.r, color.g, color.b, color.a };
    if (ImGui::ColorEdit4(title, f)) {
        color.r = f[0];
        color.g = f[1];
        color.b = f[2];
        color.a = f[3];
        return true;
    }
    return false;
}

bool ColorEditor(const char* title, ugine::ColorRGB& color) {
    float f[3] = { color.r, color.g, color.b };
    if (ImGui::ColorEdit3(title, f)) {
        color.r = f[0];
        color.g = f[1];
        color.b = f[2];
        return true;
    }
    return false;
}

bool TransformationEditor(const char* name, ugine::Transformation& transformation) {
    PropertyTable table{ name };

    auto changed{ table.EditProperty("Position", transformation.position, false, -999999.9f, 999999.9f, 0.1f) };

    table.BeginProperty("Rotation rotation");
    if (QuatEditor("##Rotation", transformation.rotation)) {
        changed = true;
    }
    table.EndProperty();

    table.BeginProperty("Scale");
    static bool uniform{ false }; // TODO:
    //ImGui::Checkbox("Uniform", &uniform);
    //if (!uniform) {
    if (Vec3Editor("##Scale", transformation.scale, false, 0.0000001f, std::numeric_limits<float>::max())) {
        changed = true;
    }
    //} else {
    //    float scale{ transformation.scale.x };
    //    if (ImGui::DragFloat("##Scale", &scale, 0.01f)) {
    //        transformation.scale = { scale, scale, scale };
    //        changed = true;
    //    }
    //}
    table.EndProperty();

    return changed;
}

bool FilePathEditor(const char* id, const char* label, Path& path, const Vector<String>& filter) {
    ImScope::Id sid(id);

    bool ok{};
    if (ImGui::Button(ICON_FA_FOLDER_OPEN)) {
        auto file{ OpenFileDialog(nullptr, filter, false) };
        if (!file.Empty()) {
            path = file[0];
            ok = true;
        }
    }

    if (label) {
        ImGui::SameLine();
        ImGui::Text(label);
    }

    return ok;
}

bool MultipleFilePathEditor(const char* id, const char* label, Vector<Path>& paths, const Vector<String>& filter) {
    ImScope::Id sid(id);

    bool ok{};
    if (ImGui::Button(ICON_FA_FOLDER_OPEN)) {
        auto res{ OpenFileDialog(nullptr, filter, true) };
        if (!res.Empty()) {
            paths = std::move(res);
            ok = true;
        }
    }

    if (label) {
        ImGui::SameLine();
        ImGui::Text(label);
    }

    return ok;
}

void LoadingIndicatorCircle(const char* label, float indicatorRadius, const ImVec4& mainColor, const ImVec4& bgColor, int circleCount, float speed) {
    ImGuiWindow* window{ GetCurrentWindow() };
    if (window->SkipItems) {
        return;
    }

    ImGuiContext& g{ *GImGui };
    const ImGuiID id{ window->GetID(label) };

    const auto pos{ window->DC.CursorPos };
    const auto spacing{ ImGui::GetStyle().ItemSpacing };
    const float circleRadius{ indicatorRadius / 10.0f };
    const ImRect bb(pos, ImVec2(pos.x + indicatorRadius * 2.0f, pos.y + spacing.y + indicatorRadius * 2.0f));

    const auto innerRadius{ indicatorRadius - circleRadius * 4.0f };

    ItemSize(bb, 5);
    if (!ItemAdd(bb, id)) {
        return;
    }

    const float t{ float(g.Time * speed) };
    const auto degree_offset{ 2.0f * IM_PI / circleCount };
    const ImVec2 center{ pos.x + spacing.x / 2.0f + innerRadius, pos.y + spacing.y + innerRadius };

    for (int i{}; i < circleCount; ++i) {
        const auto x{ innerRadius * std::sin(degree_offset * i) };
        const auto y{ innerRadius * std::cos(degree_offset * i) };
        const auto growth{ std::max(0.0f, std::sin(t - i * degree_offset)) };
        ImVec4 color{};
        color.x = mainColor.x * growth + bgColor.x * (1.0f - growth);
        color.y = mainColor.y * growth + bgColor.y * (1.0f - growth);
        color.z = mainColor.z * growth + bgColor.z * (1.0f - growth);
        color.w = 1.0f;
        window->DrawList->AddCircleFilled(ImVec2(center.x + x, center.y - y), circleRadius + growth * circleRadius, GetColorU32(color));
    }
}

void Loading(const char* label, float size) {
    LoadingIndicatorCircle(label, size, ImGui::GetStyle().Colors[ImGuiCol_Text], ImGui::GetStyle().Colors[ImGuiCol_TextDisabled], 10, 1.0f);
}

bool StringEditor(const char* title, String& string) {
    char text[256]; // TODO:
    strcpy(text, string.Data());
    if (ImGui::InputText(title, text, 256)) {
        string = text;
        return true;
    }

    return false;
}

bool ImageListItem(const char* label, ImTextureID image, const ImVec2& size, bool border, bool* selected) {
    // Based on https://github.com/dfranx/ImFileDialog/blob/main/ImFileDialog.cpp.
    const auto TextLines{ 2 };

    const auto& style{ ImGui::GetStyle() };
    const auto& g{ *ImGui::GetCurrentContext() };
    const auto* window{ g.CurrentWindow };
    const auto pos{ window->DC.CursorPos };

    const auto& padding{ style.FramePadding };
    const auto& spacing{ style.ItemInnerSpacing };

    const ImVec2 innerSize{ size.x - 2 * padding.x, size.y - 2 * padding.y };
    const ImVec2 innerPos{ pos.x + padding.x, pos.y + padding.y };

    const auto windowSpace{ ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x };

    const auto iconSize{ innerSize.y - spacing.y - g.FontSize * TextLines };
    const auto iconPosX{ innerPos.x + (innerSize.x - iconSize) / 2.0f };

    const auto textSize{ ImGui::CalcTextSize(label, 0, true, innerSize.x) };

    // State.
    bool ret{};
    if (ImGui::InvisibleButton(label, size)) {
        ret = true;
    }

    const auto hovered{ ImGui::IsItemHovered() };
    const auto active{ ImGui::IsItemActive() };
    const auto doubleClick{ ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) };
    if (doubleClick && hovered) {
        ret = true;
    }

    const auto isSelected{ selected ? *selected : false };
    if (hovered || active || isSelected) {
        const auto color{ ImGui::ColorConvertFloat4ToU32(
            ImGui::GetStyle().Colors[active ? ImGuiCol_HeaderActive : (isSelected ? ImGuiCol_Header : ImGuiCol_HeaderHovered)]) };

        window->DrawList->AddRectFilled(pos, ImVec2{ pos.x + size.x, pos.y + size.y }, color);
    }

    const auto textColor{ ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[ImGuiCol_Text]) };

    {
        ImVec2 availSize{ ImVec2(innerSize.x, iconSize) };
        const float scale{ std::min<float>(availSize.x / innerSize.x, availSize.y / innerSize.y) };
        availSize.x = innerSize.x * scale;
        availSize.y = innerSize.y * scale;

        const ImVec2 borderStart{ innerPos.x + (innerSize.x - availSize.x) / 2.0f, innerPos.y + (iconSize - availSize.y) / 2.0f };
        const ImVec2 borderEnd{ borderStart.x + availSize.x, borderStart.y + availSize.y };

        if (border) {
            const ImVec2 previewStart{ borderStart.x + 4, borderStart.y + 4 };
            const ImVec2 previewEnd{ borderStart.x + availSize.x - 4, borderStart.y + availSize.y - 4 };

            //window->DrawList->AddRectFilled(borderStart, borderEnd, textColor);
            window->DrawList->AddRectFilled(ImVec2{ previewStart.x, previewEnd.y }, ImVec2{ previewEnd.x, previewEnd.y + 4 }, textColor);

            window->DrawList->AddImage(image, previewStart, previewEnd);
        } else {
            window->DrawList->AddImage(image, borderStart, borderEnd);
        }
    }

    {
        window->DrawList->AddText(
            g.Font, g.FontSize, ImVec2(innerPos.x + (innerSize.x - textSize.x) / 2.0f, innerPos.y + iconSize + spacing.y), textColor, label, 0, innerSize.x);
    }

    // Item aligning.
    const float lastButtomPos{ ImGui::GetItemRectMax().x };
    const float thisButtonPos{ lastButtomPos + spacing.x + size.x };
    if (thisButtonPos < windowSpace) {
        ImGui::SameLine();
    }

    if (selected) {
        *selected = ret;
    }

    return ret;
}

void ToolTipText(ugine::StringView text) {
    if (ImGui::IsItemHovered()) {
        if (ImGui::BeginTooltip()) {
            ImGui::TextUnformatted(text.Data());
            ImGui::EndTooltip();
        }
    }
}

void CVarEdit(ugine::CVar& var) {
    ImGui::BeginGroup();
    switch (var.type) {
    case ugine::CVar::Type::Bool: ImGui::Checkbox(var.name, &var.value.b); break;
    case ugine::CVar::Type::Int:
        ImGui::TextUnformatted(var.name);
        ImGui::SameLine();
        ImGui::SliderInt(var.name, &var.value.i, var.minValue.i, var.maxValue.i);
        break;
    case ugine::CVar::Type::Float:
        ImGui::TextUnformatted(var.name);
        ImGui::SameLine();
        ImGui::SliderFloat(var.name, &var.value.f, var.minValue.f, var.maxValue.f);
        break;
    }
    ImGui::EndGroup();

    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(var.description);
        ImGui::EndTooltip();
    }
}

void UniformValue(const ugine::UniformValue& value) {
    switch (value.type) {
        using enum ugine::UniformValue::Type;

    case Float: ImGui::Text("%.02f", value.Get<float>()); break;
    case Float2: ImGuiEx::Text("%.02f, %.02f", value.Get<glm::vec2>()); break;
    case Float3: ImGuiEx::Text("%.02f, %.02f, %.02f", value.Get<glm::vec3>()); break;
    case Float4: ImGuiEx::Text("%.02f, %.02f, %.02f, %.02f", value.Get<glm::vec4>()); break;
    case Int: ImGui::Text("%d", value.Get<i32>()); break;
    case Int2: ImGuiEx::Text("%d, %d", value.Get<glm::ivec2>()); break;
    case Int3: ImGuiEx::Text("%d, %d, %d", value.Get<glm::ivec3>()); break;
    case Int4: ImGuiEx::Text("%d, %d, %d, %d", value.Get<glm::ivec4>()); break;
    default: ImGui::TextUnformatted("<default_value>");
    }
}

bool BeginToolbar(int id, ImVec2 size) {
    ImGuiWindowFlags flags{};
    flags |= ImGuiWindowFlags_NoDocking;
    flags |= ImGuiWindowFlags_NoTitleBar;
    flags |= ImGuiWindowFlags_NoResize;
    flags |= ImGuiWindowFlags_NoMove;
    flags |= ImGuiWindowFlags_NoScrollbar;
    flags |= ImGuiWindowFlags_NoSavedSettings;

    return ImGui::BeginChildFrame(id, size, flags);
}

void EndToolbar() {
    ImGui::EndChildFrame();
}

} // namespace ImGuiEx