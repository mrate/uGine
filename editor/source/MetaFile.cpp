#include "MetaFile.h"

#include <ugine/Json.h>
#include <ugine/Log.h>

#include <ugine/engine/core/Json.h>

#include <fstream>

namespace ugine::ed {

Path MetaFile::MetaPathFromResourcePath(const Path& resourcePath) {
    auto metaPath{ resourcePath };

    String extension{ resourcePath.Extension() };
    extension.Append(".meta");

    metaPath.ReplaceExtension(extension);
    return metaPath;
}

MetaFile MetaFile::Create(
    const Path& resourcePath, const ResourceID& id, const ResourceType type, std::optional<Path> origin, std::optional<nlohmann::json> meta) {
    const auto metaFile{ MetaPathFromResourcePath(resourcePath) };
    UGINE_DEBUG("Creating meta file '{}'...", metaFile.String());

    nlohmann::json json{};
    json["id"] = id;
    json["type"] = type.Name();
    if (origin) {
        json["origin"] = std::string{ origin->Data() };
    }

    if (meta) {
        json["meta"] = meta.value();
    }

    std::ofstream out{ metaFile.Data() };
    out << json;

    MetaFile res{};
    res.id_ = id;
    res.type_ = type;
    res.resourcePath_ = metaFile;
    res.resourcePath_.ReplaceExtension("");
    res.origin_ = origin.value_or(Path{});
    res.meta_ = meta.value_or(nlohmann::json{});

    return res;
}

void MetaFile::Delete(const Path& resourcePath) {
    if (resourcePath.Empty()) {
        return;
    }

    const auto metaPath{ MetaPathFromResourcePath(resourcePath) };
    if (!FileSystem::Remove(metaPath)) {
        UGINE_ERROR("Failed to delete meta file '{}'", resourcePath.String());
    }
    
    if (!FileSystem::Remove(resourcePath)) {
        UGINE_ERROR("Failed to delete file '{}'", resourcePath.String());
    }
}

MetaFile MetaFile::OpenResource(const Path& resourcePath) {
    try {
        return MetaFile{ MetaPathFromResourcePath(resourcePath) };
    } catch (...) {
        return MetaFile{};
    }
}

MetaFile::MetaFile(const Path& metaPath) {
    nlohmann::json json;

    {
        std::ifstream in{ metaPath.Data() };
        in >> json;
    }

    id_ = json["id"].get<ResourceID>();
    type_ = ResourceType{ json["type"].get<std::string>() };
    resourcePath_ = metaPath;
    resourcePath_.ReplaceExtension("");
    origin_ = Path{ json.value<std::string>("origin", "") };

    if (json.contains("meta")) {
        meta_ = json["meta"];
    }
}

} // namespace ugine::ed