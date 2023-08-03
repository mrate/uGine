#pragma once

#include <ugine/engine/gfx/asset/SerializedModel.h>

#include <optional>

namespace ugine::ed {

class MaterialCreator {
public:
    explicit MaterialCreator(StringView name);

    void AddTexture(SerializedTextureMapping mapping);
    void SetColor(SerializedColorMapping mapping, const glm::vec4& color);

    Path Build(const Path& targetDir);

    std::optional<SerializedTextureMapping> GetMappingForBinding(uint32_t binding) const {
        const auto it{ bindingToMapping_.find(binding) };
        return it == bindingToMapping_.end() ? std::nullopt : std::make_optional(it->second);
    }

private:
    String name_{};

    bool hasAlbedo_{};
    bool hasNormal_{};
    bool hasEmissive_{};

    std::optional<glm::vec4> diffuseColor_;
    std::optional<glm::vec4> emissiveColor_;

    std::map<uint32_t, SerializedTextureMapping> bindingToMapping_;
};

} // namespace ugine::ed