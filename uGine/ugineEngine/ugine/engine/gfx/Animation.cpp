#include "Animation.h"

#include "asset/SerializedAnimation.h"

#include <ugine/Profile.h>
#include <ugine/Ugine.h>
#include <ugine/Vector.h>

#include <ugine/engine/core/ResourceManager.h>
#include <ugine/engine/gfx/Model.h>
#include <ugine/engine/math/Math.h>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

namespace ugine {

bool Animation::HandleLoad(Span<const u8> data) {
    PROFILE_EVENT();

    SerializedAnimation serialized{};
    if (!LoadAnimation(data, serialized)) {
        return false;
    }

    lengthSeconds = serialized.lengthSeconds;
    name = serialized.name;
    for (const auto& [name, channel] : serialized.channels) {
        channels.insert(std::make_pair(StringID{ name.c_str() },
            Animation::Channel{
                .positions = channel.positions,
                .rotations = channel.rotations,
                .scales = channel.scales,
            }));
    }

    return true;
}

bool Animation::HandleUnload() {
    PROFILE_EVENT();

    name.clear();
    lengthSeconds = 0;
    channels.clear();
    return true;
}

// Animation:
template <typename T> inline T Interpolate(f32 t, const std::vector<std::pair<f32, T>>& values) {
    if (values.size() == 1) {
        return values[0].second;
    }

    std::pair<f32, T> start{};
    std::pair<f32, T> end{};

    for (u32 i = 0; i < values.size(); ++i) {
        if (i == values.size() - 1) {
            // At the end.
            start = values[i];
            end = values[0];
            break;
        } else if (t < values[i + 1].first) {
            start = values[i];
            end = values[i + 1];
            break;
        }
    }

    UGINE_ASSERT(end.first != start.first);
    const auto p{ (t - start.first) / (end.first - start.first) };
    return Interpolate(start.second, end.second, p);
}

glm::mat4 InterpolateChannel(f32 time, const Animation::Channel& channel) {
    const auto position{ Interpolate(time, channel.positions) };
    const auto rotation{ Interpolate(time, channel.rotations) };
    const auto scale{ Interpolate(time, channel.scales) };

    return glm::translate(position) * glm::mat4(rotation) * glm::scale(scale);
}

glm::mat4 InterpolateChannels(f32 time1, const Animation::Channel& channel1, f32 time2, const Animation::Channel& channel2, f32 blend) {
    const auto position1{ Interpolate(time1, channel1.positions) };
    const auto rotation1{ Interpolate(time1, channel1.rotations) };
    const auto scale1{ Interpolate(time1, channel1.scales) };

    const auto position2{ Interpolate(time2, channel2.positions) };
    const auto rotation2{ Interpolate(time2, channel2.rotations) };
    const auto scale2{ Interpolate(time2, channel2.scales) };

    const auto position{ Interpolate(position1, position2, blend) };
    const auto rotation{ Interpolate(rotation1, rotation2, blend) };
    const auto scale{ Interpolate(scale1, scale2, blend) };

    return glm::translate(position) * glm::mat4(rotation) * glm::scale(scale);
}

void UpdateAnimation(IAllocator& allocator, const Model& model, const Animation& animation, f32 time, Span<glm::mat4> outMatrices) {
    UGINE_ASSERT(!model.Nodes().Empty());
    UGINE_ASSERT(!model.Bones().Empty());

    //for (u32 i{}; i < outMatricesCount; ++i) {
    //    outMatrices[i] = glm::mat4{ 1.0f };
    //}
    //return;

    struct Entry {
        u32 nodeIndex;
        glm::mat4 parentTransform{ 1.0f };
    };

    Vector<Entry> nodeStack{ model.Nodes().Size(), allocator };
    u32 stackTop{};

    nodeStack[stackTop++] = Entry{ 0, glm::mat4{ 1.0f } };

    while (stackTop > 0) {
        Entry entry{ nodeStack[--stackTop] };

        const auto& node{ model.Nodes()[entry.nodeIndex] };

        const auto channelIt{ animation.channels.find(node.id) };
        const glm::mat4 nodeTransform{ channelIt != animation.channels.end() ? InterpolateChannel(time, channelIt->second) : node.transform };
        const auto globalTransform{ entry.parentTransform * nodeTransform };

        if (node.boneIndex != Model::INVALID_INDEX) {
            UGINE_ASSERT(node.boneIndex < outMatrices.Size());
            const auto boneMatrix{ model.GlobalInverseTransform() * globalTransform * model.Bones()[node.boneIndex].offsetMatrix };
            outMatrices[node.boneIndex] = boneMatrix;
        }

        for (const auto child : node.children) {
            nodeStack[stackTop++] = Entry{ child, globalTransform };
        }
    }
}

void UpdateAnimation(IAllocator& allocator, const Model& model, const Animation& animation1, f32 time1, const Animation& animation2, f32 time2, f32 blend,
    Span<glm::mat4> outMatrices) {
    UGINE_ASSERT(!model.Nodes().Empty());
    UGINE_ASSERT(!model.Bones().Empty());

    //for (u32 i{}; i < outMatricesCount; ++i) {
    //    outMatrices[i] = glm::mat4{ 1.0f };
    //}
    //return;

    struct Entry {
        u32 nodeIndex{};
        glm::mat4 parentTransform{ 1.0f };
    };

    Vector<Entry> nodeStack{ model.Nodes().Size(), allocator };
    u32 stackTop{};

    nodeStack[stackTop++] = Entry{ 0, glm::mat4{ 1.0f } };

    while (stackTop > 0) {
        Entry entry{ nodeStack[--stackTop] };

        const auto& node{ model.Nodes()[entry.nodeIndex] };

        const auto channel1It{ animation1.channels.find(node.id) };
        const auto channel2It{ animation2.channels.find(node.id) };

        //const glm::mat4 nodeTransform{ channelIt != animation.channels.end() ? InterpolateChannel(time, channelIt->second) : node.transform };

        const auto nodeTransform{ [&] {
            if (channel1It != animation1.channels.end() && channel2It != animation2.channels.end()) {
                return InterpolateChannels(time1, channel1It->second, time2, channel2It->second, blend);
            } else {
                return node.transform;
            }
        }() };

        const auto globalTransform{ entry.parentTransform * nodeTransform };

        if (node.boneIndex != Model::INVALID_INDEX) {
            UGINE_ASSERT(node.boneIndex < outMatrices.Size());
            const auto boneMatrix{ model.GlobalInverseTransform() * globalTransform * model.Bones()[node.boneIndex].offsetMatrix };
            outMatrices[node.boneIndex] = boneMatrix;
        }

        for (const auto child : node.children) {
            nodeStack[stackTop++] = Entry{ child, globalTransform };
        }
    }
}

//
//Animation AnimationLoader::Create(Engine& engine, const std::filesystem::path& path, const SerializedAnimation& animation) {
//    return Animation::Create(ResourceID::Generate(), engine.GetResources(), animation);
//}
//
//Animation AnimationLoader::Load(const ResourceID& id, Engine& engine) {
//    const auto path{ engine.GetResources().ResourcePath(id) };
//    if (path.empty()) {
//        UGINE_THROW(Error, std::format("Failed to load animation {}", id.uuid.ToString()).c_str());
//    }
//
//    auto animation{ LoadSerialized<SerializedAnimation>(path) };
//    return Animation::Create(id, engine.GetResources(), animation);
//}
//
//void AnimationLoader::Delete(Animation& animation, Engine& engine) {
//}

} // namespace ugine