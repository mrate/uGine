#pragma once

#include <ugine/Span.h>
#include <ugine/Ugine.h>
#include <ugine/Vector.h>

#include <glm/glm.hpp>

#include <map>
#include <string>
#include <vector>

namespace ugine {

struct SerializedAnimation {
    struct Channel {
        std::vector<std::pair<f32, glm::vec3>> positions;
        std::vector<std::pair<f32, glm::fquat>> rotations;
        std::vector<std::pair<f32, glm::vec3>> scales;
    };

    std::string name;
    f32 lengthSeconds;
    std::map<std::string, Channel> channels;
};

bool LoadAnimation(Span<const u8> in, SerializedAnimation& out);
bool SaveAnimation(const SerializedAnimation& in, Vector<u8>& out);

} // namespace ugine