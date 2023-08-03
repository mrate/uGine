#pragma once

#include <ugine/Json.h>
#include <ugine/Log.h>
#include <ugine/Path.h>

#include <ugine/engine/core/Resource.h>

namespace ugine::ed {

class MetaFile {
public:
    static Path MetaPathFromResourcePath(const Path& resourcePath);
    static MetaFile Create(const Path& resourcePath, const ResourceID& id, const ResourceType type,
        std::optional<Path> origin = {}, std::optional<nlohmann::json> meta = {});
    static void Delete(const Path& resourcePath);
    static MetaFile OpenResource(const Path& resourcePath);

    MetaFile() = default;
    explicit MetaFile(const Path& metaPath);

    const ResourceType& Type() const { return type_; }
    const ResourceID& Id() const { return id_; }
    const Path& ResourcePath() const { return resourcePath_; }
    const Path& Origin() const { return origin_; }
    const nlohmann::json& Meta() const { return meta_; }

private:
    ResourceID id_{};
    ResourceType type_{};
    Path resourcePath_{};
    Path origin_{};
    nlohmann::json meta_;
};

} // namespace ugine::ed