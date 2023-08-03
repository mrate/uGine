#pragma once

#include "../EditorEvents.h"
#include "../EditorTool.h"

#include <ugine/engine/core/Resource.h>

#include <ugine/engine/gfx/Texture.h>

#include <ugine/Vector.h>

#include <algorithm>

namespace ugine::ed {

class EditorContext;

class PreviewTextureWindow : public EditorTool {
public:
    explicit PreviewTextureWindow(EditorContext& context);

    void Add(ResourceHandle<Texture> texture);
    void Show() { visible_ = true; }

    void Render() override;

private:
    struct TextureView {
        ResourceHandle<Texture> resource;
        String name;
    };

    void OnOpenResource(const OpenResourceEvent<Texture>& resource);
    void OnOpenTypelessResource(const OpenTypelessResourceEvent& event);
    void OnContextMenu(const ResourceContextMenuEvent<Texture>& event);

    Vector<TextureView> textures_;
    std::optional<ResourceID> focus_;
};

} // namespace ugine::ed