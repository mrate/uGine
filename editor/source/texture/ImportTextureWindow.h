#pragma once

#include "../EditorTool.h"
#include "../window/Window.h"

#include <ugine/engine/core/Resource.h>

namespace ugine::ed {

class EditorContext;
struct ReloadResourceEvent;

class ImportTextureWindow : public ModalButtonWindow {
public:
    explicit ImportTextureWindow(EditorContext& context);

    const char* Name() const override { return "Import texture"; }
    void Init() override;
    void Show() override;

private:
    bool ImportTextures();
    void CubeMapOrdering();
    void ReorderCubeMap();

    void ReloadTextures(const ResourceID& id);
    void OnReloadResource(const ReloadResourceEvent& event);

    EditorContext& context_;
    bool isCubeMap_{};
    Vector<Path> sourcePaths_;
    Path targetPath_;
};

} // namespace ugine::ed