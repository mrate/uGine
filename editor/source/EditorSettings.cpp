#include "EditorSettings.h"

#include <fstream>

namespace ugine::ed {

void EditorSettings::Load(const Path& path) {
    std::ifstream in{ path.Data(), std::ios::binary };
    try {
        in >> json_;
    } catch (...) {
        UGINE_ERROR("Failed to load editor settings '{}'", path.String());
    }
}

void EditorSettings::Save(const Path& path) {
    std::ofstream out{ path.Data() };
    try {
        out << json_;
    } catch (...) {
        UGINE_ERROR("Failed to save editor settings '{}'", path.String());
    }
}

nlohmann::json& EditorSettings::GetPath(StringView name) {
    // TODO:
    return json_[name.Data()];
}

} // namespace ugine::ed