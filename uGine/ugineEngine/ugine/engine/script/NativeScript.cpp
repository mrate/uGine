#include "NativeScript.h"

#include <ugine/Error.h>
#include <ugine/Ugine.h>

namespace ugine {

Transformation NativeScript::LocalTransformation() const {
    UGINE_ASSERT(owner_.IsValid());

    return owner_.Has<TransformationComponent>() ? owner_.Component<TransformationComponent>().localTransformation : ugine::Transformation{};
}

void NativeScript::SetLocalTransformation(const ugine::Transformation& trans) {
    UGINE_ASSERT(owner_.IsValid());

    owner_.SetLocalTransformation(trans);
}

void NativeScript::LookAt(const glm::vec3& to) {
    UGINE_ASSERT(owner_.IsValid());
    UGINE_THROW(Error, "Not implemented");
}

void NativeScript::LookAt(const GameObject& go) {
    UGINE_ASSERT(owner_.IsValid());
    UGINE_THROW(Error, "Not implemented");
}

} // namespace ugine
