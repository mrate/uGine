#include "ResourceThumbnails.h"

#include "../EditorContext.h"

#include <ugine/Profile.h>

namespace ugine::ed {

gfxapi::TextureHandle ResourceThumbnails::Get(const ResourceID& id) {
    using namespace gfxapi;

    auto it{ cache_.find(id) };
    if (it == cache_.end()) {
        Thumbnail thumbnail{};
        CreateEvent(id, thumbnail);
        it = cache_.insert(std::make_pair(id, thumbnail)).first;
    }

    if (!it->second.valid) {
        UpdateEvent(id, it->second.texture);
        it->second.valid = true;
    }

    return it->second.texture;
}

ResourceThumbnailCreator::ResourceThumbnailCreator(EditorContext& context)
    : context_{ context }
    , thumbnailWorldPreview_{ context, false } {
    extent_ = gfxapi::Extent2D{ u32(context_.ThumbnailSize().x), u32(context_.ThumbnailSize().y) };
    thumbnailWorldPreview_.Resize(extent_.width, extent_.height);

    icons_[Animation::TYPE] = context_.Assets().AnimationIcon->Get();
    icons_[WorldDescriptor::TYPE] = context_.Assets().WorldIcon->Get();
    icons_[Shader::TYPE] = context_.Assets().ShaderIcon->Get();
    icons_[LuaScript::TYPE] = context_.Assets().ScriptIcon->Get();

    context_.Thumbnails().Connect<ResourceThumbnails::CreateThumbnail, &ResourceThumbnailCreator::ThumbnailCreate>(this);
    context_.Thumbnails().Connect<ResourceThumbnails::UpdateThumbnail, &ResourceThumbnailCreator::ThumbnailUpdate>(this);
}

ResourceThumbnailCreator::~ResourceThumbnailCreator() {
}

void ResourceThumbnailCreator::Update() {
    if (!activeUpdate_.IsNull()) {
        activeUpdate_ = ResourceID{};
        thumbnailWorldPreview_.SetRendering(false);
    }

    if (pendingUpdates_.empty()) {
        return;
    }

    PROFILE_EVENT_NC("Update editor thumbnails", COLOR_EDITOR_PROFILE);

    for (auto it{ pendingUpdates_.begin() }; it != pendingUpdates_.end(); ++it) {
        auto& update{ *it };

        if (it->resource->Ready()) {
            it->render();
            activeUpdate_ = update.resource->Id();
            thumbnailWorldPreview_.SetRendering(true);
            pendingUpdates_.erase(it);
            break;
        }
    }
}

void ResourceThumbnailCreator::ThumbnailCreate(const ResourceThumbnails::CreateThumbnail& event) {
    const auto type{ context_.Engine().GetResources().ResourceType(event.resource) };

    const auto it{ icons_.find(type) };
    if (it != icons_.end()) {
        *event.thumbnail = it->second;
        *event.owned = false;
    } else {
        *event.thumbnail = CreateThumbnailTexture();
        *event.owned = true;
    }
}

void ResourceThumbnailCreator::ThumbnailUpdate(const ResourceThumbnails::UpdateThumbnail& event) {
    const auto type{ context_.Engine().GetResources().ResourceType(event.resource) };

    if (type == Material::TYPE) {
        auto material{ context_.Engine().GetResources().Get<Material>(event.resource) };

        pendingUpdates_.push_back(UpdateContext{
            .resource = material,
            .render =
                [this, event, material] {
                    thumbnailWorldPreview_.SetOutput(event.thumbnail, extent_);
                    thumbnailWorldPreview_.SetRendering(true);
                    thumbnailWorldPreview_.SetMaterial(material);
                    thumbnailWorldPreview_.SetMaterialPreviewCamera();
                },
        });
    } else if (type == Model::TYPE) {
        auto model{ context_.Engine().GetResources().Get<Model>(event.resource) };

        pendingUpdates_.push_back(UpdateContext{
            .resource = model,
            .render =
                [this, event, model] {
                    thumbnailWorldPreview_.SetOutput(event.thumbnail, extent_);
                    thumbnailWorldPreview_.SetRendering(true);
                    thumbnailWorldPreview_.SetModel(model);
                    thumbnailWorldPreview_.SetModelPreviewCamera();
                },
        });
    }
}

gfxapi::TextureHandle ResourceThumbnailCreator::CreateThumbnailTexture(const gfxapi::SubresourceData* data, gfxapi::TextureLayout layout) {
    using namespace gfxapi;

    auto state{ context_.Engine().GetState<GraphicsState>() };

    return state->device.CreateTexture(
        TextureDesc{
            .name = "MaterialThumbnail",
            .extent = extent_,
            .format = Format::R8G8B8A8_Unorm,
            .usage = TextureUsageFlags::RenderTarget | TextureUsageFlags::Sampled,
        },
        layout, data ? *data : ArrayProxy<SubresourceData>{});
}

} // namespace ugine::ed