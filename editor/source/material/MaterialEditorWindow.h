#pragma once

#include "../EditorContext.h"
#include "../EditorEvents.h"
#include "../window/EditorWindow.h"

#include "../window/Window.h"

#include "../widgets/PropertyTable.h"
#include "../widgets/ResourceSelectWidget.h"

#include "MaterialPreviewRenderer.h"

#include <ugine/engine/engine/Engine.h>
#include <ugine/engine/gfx/GraphicsState.h>
#include <ugine/engine/gfx/Material.h>

namespace ugine::ed {

class MaterialEditorWindow : public EditorWindow {
public:
    explicit MaterialEditorWindow(EditorContext& context, StringView id);
    void SetMaterial(ResourceHandle<Material> material);

private:
    bool Valid() const override;
    void Content() override;
    void Reload() override;
    bool Save() override;
    void Closed() override;
    ImVec2 DefaultSize() const override { return ImVec2{ 1000, 600 }; }

    void OnOpenMaterial(const OpenResourceEvent<Material> event) { SetMaterial(event.resource); }
    void OnOpenTypelessResource(const OpenTypelessResourceEvent& event);

    //void CreateNewMaterial();

    void Populate();

    void RenderMaterial();
    void RenderPreview();

    MaterialPreviewRenderer preview_;

    ResourceHandle<Material> material_;
};

} // namespace ugine::ed