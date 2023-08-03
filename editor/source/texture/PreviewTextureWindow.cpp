#include "PreviewTextureWindow.h"
#include "../EditorContext.h"

#include <ugine/Profile.h>

namespace ugine::ed {

PreviewTextureWindow::PreviewTextureWindow(EditorContext& context)
    : EditorTool{ context, false }
    , textures_{ context.Allocator() } {

    auto& view{ context_.MainMenu().Get("View") };
    view.AddAction([this] { return ImGui::Checkbox("Texture preivew", &visible_); });

    context_.Events().Connect<OpenResourceEvent<Texture>, &PreviewTextureWindow::OnOpenResource>(this);
    context_.Events().Connect<OpenTypelessResourceEvent, &PreviewTextureWindow::OnOpenTypelessResource>(this);
}

void PreviewTextureWindow::Add(ResourceHandle<Texture> texture) {
    const auto name{ context_.Engine().GetResources().ResourceName(texture->Id()) };

    const auto index{ textures_.FindIf([&](const auto& t) { return t.resource->Id() == texture->Id(); }) };
    if (index < 0) {
        textures_.PushBack(TextureView{
            .resource = texture,
            .name = name,
        });
    }

    focus_ = texture->Id();
    Show();
}

void PreviewTextureWindow::Render() {
    PROFILE_EVENT_NC("PreviewTextureWindow", COLOR_EDITOR_PROFILE);

    std::optional<ResourceID> toRemove;

    if (ImGui::Begin("Texture preview", &visible_)) {
        if (!visible_) {
            textures_.Clear();
        }

        if (ImGui::BeginTabBar("Textures")) {
            for (const auto& texture : textures_) {
                bool opened{ true };
                int flags{};
                if (focus_ && texture.resource->Id() == focus_) {
                    flags = ImGuiTabItemFlags_SetSelected;
                    focus_ = std::nullopt;
                }

                if (ImGui::BeginTabItem(std::format("{}##{}", texture.name, texture.resource->Id().uuid.ToString()).c_str(), &opened, flags)) {
                    switch (texture.resource->State()) {
                    case ResourceState::Unloaded: ImGuiEx::Loading(); break;
                    case ResourceState::Loaded:
                        {
                            auto state{ context_.Engine().GetState<GraphicsState>() };

                            UGINE_ASSERT(state);
                            auto desc{ state->device.GetTextureDesc(texture.resource->Get()) };
                            const auto isCube{ (desc.misc & gfxapi::TextureMiscFlags::Cube) != 0 };

                            ImGui::Text(" Width: %d", desc.extent.width);
                            ImGui::Text("Height: %d", desc.extent.height);
                            ImGui::Text(" Depth: %d", desc.extent.depth);

                            if (isCube) {
                                ImGui::Text("TODO: Cubemaps");
                                //ImGui::Image(ImGuiTexture(texture.resource->GetBindlessIndex(), 1), ImVec2{ 256, 256 });
                            } else {
                                ImGui::Image(ImGuiTexture(texture.resource->GetBindlessIndex()), ImVec2{ 256, 256 });
                            }
                        }

                        break;
                    case ResourceState::Failed:
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ 1, 0, 0, 1 });
                        ImGui::Text("Resource error");
                        ImGui::PopStyleColor();
                        break;
                    }
                    ImGui::EndTabItem();
                }

                if (!opened) {
                    toRemove = texture.resource->Id();
                }
            }

            ImGui::EndTabBar();
        }
    }

    ImGui::End();

    if (toRemove) {
        textures_.EraseIf([&](const auto& texture) { return texture.resource->Id() == *toRemove; });
    }
}

void PreviewTextureWindow::OnOpenResource(const OpenResourceEvent<Texture>& event) {
    Add(event.resource);
}

void PreviewTextureWindow::OnOpenTypelessResource(const OpenTypelessResourceEvent& event) {
    if (event.resource.type == Texture::TYPE) {
        Add(context_.Engine().GetResources().Get<Texture>(event.resource.id));
    }
}

void PreviewTextureWindow::OnContextMenu(const ResourceContextMenuEvent<Texture>& event) {
    context_.ContextMenu().Open({
        { "View", [this, resource = event.resource] { Add(resource); } },
        { "Delete", [this, resource = event.resource] { context_.DeleteResource(resource); } },
    });
}

} // namespace ugine::ed