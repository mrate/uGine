#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <ugine/Color.h>
#include <ugine/engine/gfx/ImGui.h>
#include <ugine/engine/world/Transformation.h>

#include <ugine/Path.h>
#include <ugine/String.h>

#include <optional>

namespace ugine {
struct CVar;
struct UniformValue;
} // namespace ugine

namespace ImGuiEx {

void MatrixToFloatArray(const glm::mat4& mat, float* array);
void FloatArrayToMatrix(const float* array, glm::mat4& mat);

bool BeginToolbar(int id, ImVec2 size);
void EndToolbar();

bool Vec2Editor(const char* title, glm::vec2& vec, float min = 0.0f, float max = 0.0f, float speed = 1.0f);
bool Vec3Editor(const char* title, glm::vec3& vec, bool asColor = false, float min = 0.0f, float max = 0.0f, float speed = 1.0f);
bool Vec4Editor(const char* title, glm::vec4& vec, bool asColor = false, float min = 0.0f, float max = 0.0f, float speed = 1.0f);
bool QuatEditor(const char* title, glm::fquat& q, float min = 0.0f, float max = 0.0f, float speed = 1.0f);
bool ColorEditor(const char* title, ugine::ColorRGB& color);
bool ColorEditor(const char* title, ugine::ColorRGBA& color);
bool TransformationEditor(const char* name, ugine::Transformation& transformation);
bool StringEditor(const char* title, ugine::String& string);

bool FilePathEditor(const char* id, const char* label, ugine::Path& path, const ugine::Vector<ugine::String>& filter);
bool MultipleFilePathEditor(const char* id, const char* label, ugine::Vector<ugine::Path>& paths, const ugine::Vector<ugine::String>& filter);

void LoadingIndicatorCircle(
    const char* label, float indicator_radius, const ImVec4& main_color, const ImVec4& backdrop_color, int circle_count = 10, float speed = 1.0f);
void Loading(const char* label = "Loading", float size = 64.0f);

bool ImageListItem(const char* label, ImTextureID image, const ImVec2& size, bool border, bool* selected = nullptr);

void ToolTipText(ugine::StringView text);

template <typename T> void ToolTip(T f) {
    if (ImGui::IsItemHovered()) {
        if (ImGui::BeginTooltip()) {
            f();
            ImGui::EndTooltip();
        }
    }
}

void CVarEdit(ugine::CVar& var);

void UniformValue(const ugine::UniformValue& value);

inline void Text(const char* msg) {
    ImGui::TextUnformatted(msg);
}
inline void Text(const ugine::Path& msg) {
    ImGui::TextUnformatted(msg.Data());
}
inline void Text(const ugine::String& msg) {
    ImGui::TextUnformatted(msg.Data());
}
inline void Text(ugine::StringView msg) {
    ImGui::TextUnformatted(msg.Data());
}
inline void Text(const char* fmt, const glm::vec2& vec) {
    ImGui::Text(fmt, vec.x, vec.y);
}
inline void Text(const char* fmt, const glm::vec3& vec) {
    ImGui::Text(fmt, vec.x, vec.y, vec.z);
}
inline void Text(const char* fmt, const glm::vec4& vec) {
    ImGui::Text(fmt, vec.x, vec.y, vec.z, vec.w);
}
inline void Text(const char* fmt, const glm::ivec2& vec) {
    ImGui::Text(fmt, vec.x, vec.y);
}
inline void Text(const char* fmt, const glm::ivec3& vec) {
    ImGui::Text(fmt, vec.x, vec.y, vec.z);
}
inline void Text(const char* fmt, const glm::ivec4& vec) {
    ImGui::Text(fmt, vec.x, vec.y, vec.z, vec.w);
}
inline void Text(const char* fmt, const glm::uvec2& vec) {
    ImGui::Text(fmt, vec.x, vec.y);
}
inline void Text(const char* fmt, const glm::uvec3& vec) {
    ImGui::Text(fmt, vec.x, vec.y, vec.z);
}
inline void Text(const char* fmt, const glm::uvec4& vec) {
    ImGui::Text(fmt, vec.x, vec.y, vec.z, vec.w);
}

} // namespace ImGuiEx