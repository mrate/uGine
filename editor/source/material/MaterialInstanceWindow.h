#pragma once

#include "../window/Window.h"

#include <ugine/engine/gfx/Material.h>

namespace ugine::ed {

class EditorContext;

class MaterialInstanceWindow : public ModalButtonWindow {
public:
    explicit MaterialInstanceWindow(EditorContext& context)
        : ModalButtonWindow{ context }
        , context_{ context } {}

    const char* Name() const override { return "New material instance"; }
    void SetOrigin(ResourceHandle<Material> material, StringView name);
    void Init() override;
    void Show() override;

private:
    EditorContext& context_;

    Path targetPath_{ FileSystem::CurrentPath() };
    String fileName_{ "instance" };
    ResourceHandle<Material> origin_;
};

} // namespace ugine::ed