#pragma once

#include "../window/Window.h"

#include "../EditorTool.h"

#include <ugine/engine/engine/Engine.h>
#include <ugine/engine/gfx/asset/SerializedMaterial.h>

namespace ugine::ed {

class EditorContext;

class ImportMaterialWindow : public ModalButtonWindow {
public:
    const Vector<String> MATERIAL_FILES_FILTER{ "Material files (*.mat)", "*.mat", "All files", "*.*" };

    explicit ImportMaterialWindow(EditorContext& context);

    const char* Name() const override { return "Import material"; }
    void Init() override;
    void Show() override;

private:
    struct MaterialError {
        String error;
        Path filename;
    };

    void ImportMaterial(const Path& path);
    void ShowErrors();
    void CreateMaterial();

    EditorContext& context_;

    Path sourcePath_;
    Path targetPath_;

    SerializedMaterial material_;

    Vector<MaterialError> materialErrors_;
};

} // namespace ugine::ed