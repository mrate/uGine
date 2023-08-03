#pragma once

#include <ugine/Utils.h>

#include <imgui.h>

namespace ImScope {

struct Id {
    UGINE_NO_COPY(Id)

    explicit Id(int* id) { ImGui::PushID((*id)++); }
    explicit Id(int id) { ImGui::PushID(id); }
    explicit Id(const char* name) { ImGui::PushID(name); }
    ~Id() { ImGui::PopID(); }
};

struct Indent {
    UGINE_NO_COPY(Indent)

    explicit Indent(float i = 32.0f)
        : i_(i) {
        ImGui::Indent(i_);
    }
    ~Indent() { ImGui::Unindent(i_); }

private:
    float i_;
};

struct Color {
    UGINE_NO_COPY(Color)

    Color(ImGuiCol col, const ImVec4& value) { ImGui::PushStyleColor(col, value); }
    ~Color() { ImGui::PopStyleColor(); }
};

struct StyleVar {
    UGINE_NO_COPY(StyleVar)

    template <typename T> StyleVar(ImGuiStyleVar style, const T& value) { ImGui::PushStyleVar(style, value); }
    ~StyleVar() { ImGui::PopStyleVar(); }
};

struct ItemWidth {
    UGINE_NO_COPY(ItemWidth)

    explicit ItemWidth(float w) { ImGui::PushItemWidth(w); }
    ~ItemWidth() { ImGui::PopItemWidth(); }
};

struct Disabled {
    UGINE_NO_COPY(Disabled);

    explicit Disabled(bool disabled) { ImGui::BeginDisabled(disabled); }
    ~Disabled() { ImGui::EndDisabled(); }
};

} // namespace ImScope
