#include "SerializedAnimation.h"

#include <ugine/engine/core/Resource.h>

#include <ugine/Error.h>
#include <ugine/Log.h>
#include <ugine/Profile.h>
#include <ugine/Serialization.h>

#include <bitsery/bitsery.h>
#include <bitsery/brief_syntax.h>
#include <bitsery/brief_syntax/map.h>
#include <bitsery/brief_syntax/string.h>
#include <bitsery/brief_syntax/vector.h>
#include <bitsery/traits/string.h>
#include <bitsery/traits/vector.h>

#include <fstream>

namespace ugine {

void serialize(auto& s, SerializedAnimation::Channel& channel) {
    s(channel.positions);
    s(channel.rotations);
    s(channel.scales);
}

void serialize(auto& s, SerializedAnimation& animation) {
    s.text1b(animation.name, 128);
    s(animation.lengthSeconds);
    s(animation.channels);
}

bool LoadAnimation(Span<const u8> in, SerializedAnimation& out) {
    PROFILE_EVENT();

    auto state{ bitsery::quickDeserialization(InputAdapter{ in.Data(), in.Size() }, out) };
    return state.first == bitsery::ReaderError::NoError;
}

bool SaveAnimation(const SerializedAnimation& in, Vector<u8>& out) {
    PROFILE_EVENT();

    bitsery::quickSerialization(OutputAdapter{ out }, in);
    return true;
}

} // namespace ugine