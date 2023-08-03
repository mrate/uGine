#pragma once

#include <ugine/engine/world/GameObject.h>

namespace ugine::ed {

class EditorContext;

bool WorldShortcuts(EditorContext& context);

void DuplicateGO(EditorContext& context, GameObject go);

void AddResource(EditorContext& context, GameObject& go, const ResourceID& id, const ResourceType& type);

} // namespace ugine::ed