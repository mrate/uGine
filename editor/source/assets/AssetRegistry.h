#pragma once

#include "../EditorContext.h"
#include "../window/EditorWindow.h"

#include <ugine/engine/core/ResourceManager.h>

namespace ugine::ed {

class AssetRegistry : public EditorTool {
public:
    explicit AssetRegistry(EditorContext& context);

    void Render() override;
};

} // namespace ugine::ed