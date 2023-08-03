#pragma once

#include "WorldPreviewRenderer.h"

#include <ugine/engine/engine/Engine.h>
#include <ugine/engine/gfx/GraphicsState.h>
#include <ugine/engine/script/LuaScript.h>
#include <ugine/engine/world/World.h>

#include <gfxapi/Device.h>
#include <gfxapi/Handle.h>

#include <ugine/EventEmittor.h>

#include <unordered_map>

namespace ugine::ed {

class EditorContext;

class ResourceThumbnails : public EventEmittor {
public:
    struct CreateThumbnail {
        ResourceID resource;
        gfxapi::TextureHandle* thumbnail{};
        bool* owned{};
    };

    struct UpdateThumbnail {
        ResourceID resource;
        gfxapi::TextureHandle thumbnail{};
    };

    ResourceThumbnails(Engine& engine)
        : engine_{ engine } {}

    ~ResourceThumbnails() { Clear(); }

    void Clear() {
        auto state{ engine_.GetState<GraphicsState>() };

        for (auto [id, pair] : cache_) {
            if (pair.owned) {
                state->device.DestroyTexture(pair.texture);
            }
        }

        cache_.clear();
    }

    void Invalidate(const ResourceID& id) {
        auto it{ cache_.find(id) };
        if (it != cache_.end()) {
            it->second.valid = false;
        }
    }

    gfxapi::TextureHandle Get(const ResourceID& id);

private:
    struct Thumbnail {
        gfxapi::TextureHandle texture;
        bool owned{};
        bool valid{};
    };

    void CreateEvent(const ResourceID& id, Thumbnail& thumbnail) {
        CreateThumbnail event{
            id,
            &thumbnail.texture,
            &thumbnail.owned,
        };

        Emit(event);

        if (!event.thumbnail) {
            thumbnail.texture = *engine_.GetState<GraphicsState>()->textureNull;
            thumbnail.owned = false;
            thumbnail.valid = true;
        } else {
            thumbnail.valid = false;
        }
    }

    void UpdateEvent(const ResourceID& id, gfxapi::TextureHandle& texture) { Emit(UpdateThumbnail{ id, texture }); }

    Engine& engine_;

    std::unordered_map<ResourceID, Thumbnail> cache_;
};

class ResourceThumbnailCreator {
public:
    explicit ResourceThumbnailCreator(EditorContext& context);
    ~ResourceThumbnailCreator();

    void Update();

private:
    struct UpdateContext {
        ResourceHandleTypeless resource;
        std::function<void()> render;
    };

    void ThumbnailCreate(const ResourceThumbnails::CreateThumbnail& event);
    void ThumbnailUpdate(const ResourceThumbnails::UpdateThumbnail& event);

    gfxapi::TextureHandle CreateThumbnailTexture(const gfxapi::SubresourceData* data = nullptr, gfxapi::TextureLayout layout = gfxapi::TextureLayout::ReadOnly);

    ResourceID activeUpdate_;

    EditorContext& context_;
    std::map<ResourceType, gfxapi::TextureHandle> icons_;
    WorldPreviewRenderer thumbnailWorldPreview_;

    std::list<UpdateContext> pendingUpdates_;
    gfxapi::Extent2D extent_{};
};

} // namespace ugine::ed