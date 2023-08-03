#include "SerializedModel.h"

#include <ugine/Error.h>
#include <ugine/File.h>
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

namespace ugine {

void serialize(auto& s, SerializedModel::Vertex& vertex) {
    s(vertex.pos);
    s(vertex.normal);
    s(vertex.tangent);
    s(vertex.tex);
}

void serialize(auto& s, SerializedModel::Mesh& mesh) {
    s(mesh.name);
    s(mesh.transformation);
    s(mesh.material);
    s(mesh.indexOffset);
    s(mesh.indexCount);
    s(mesh.vertexOffset);
    s(mesh.transformation);
}

void serialize(auto& s, SerializedSkin& skin) {
    s(skin.jointIndices);
    s(skin.jointWeights);
}

void serialize(auto& s, SerializedModel::Bone& bone) {
    s(bone.name);
    s(bone.offsetMatrix);
}

void serialize(auto& s, SerializedModel::Node& node) {
    s(node.name);
    s(node.transformation);
    s(node.children);
}

void serialize(auto& s, SerializedModel& model) {
    s(model.rootTransform);
    s(model.globalInverseTransform);
    s(model.vertices);
    s(model.indices);
    s(model.meshes);
    s(model.materialIds);
    s(model.verticesSkinned);

    s(model.nodes);

    s(model.bones);
    s(model.boneNameToIndex);

    s(model.aabbMin);
    s(model.aabbMax);
}

bool LoadModel(Span<const u8> in, SerializedModel& out) {
    PROFILE_EVENT();

    auto state{ bitsery::quickDeserialization(InputAdapter{ in.Data(), in.Size() }, out) };
    return state.first == bitsery::ReaderError::NoError;
}

bool SaveModel(const SerializedModel& in, Vector<u8>& out) {
    PROFILE_EVENT();

    bitsery::quickSerialization(OutputAdapter{ out }, in);
    return true;
}

} // namespace ugine