#pragma once

#include <ugine/Memory.h>
#include <ugine/Span.h>
#include <ugine/String.h>
#include <ugine/engine/core/Resource.h>

#include <unordered_map>

namespace ugine {

class Engine;
class Model;

struct SerializedAnimation;

class Animation final : public Resource {
public:
    inline static const ResourceType TYPE{ "Animation" };

    explicit Animation(ResourceManager& manager, const ResourceID& id)
        : Resource{ manager, TYPE, id } {}

    ~Animation() { Unload(); }

    struct Channel {
        std::vector<std::pair<f32, glm::vec3>> positions;
        std::vector<std::pair<f32, glm::fquat>> rotations;
        std::vector<std::pair<f32, glm::vec3>> scales;
    };

    std::string name;
    f32 lengthSeconds{};
    std::unordered_map<StringID, Channel> channels;

private:
    bool HandleLoad(Span<const u8> data) override;
    bool HandleUnload() override;
};

glm::mat4 InterpolateChannel(f32 time, const Animation::Channel& channel);
glm::mat4 InterpolateChannels(f32 time1, const Animation::Channel& channel1, f32 time2, const Animation::Channel& channel2, f32 blend);

void UpdateAnimation(IAllocator& allocator, const Model& model, const Animation& animation, f32 time, Span<glm::mat4> outMatrices);
void UpdateAnimation(IAllocator& allocator, const Model& model, const Animation& animation1, f32 time1, const Animation& animation2, f32 time2, f32 blend,
    Span<glm::mat4> outMatrices);

} // namespace ugine