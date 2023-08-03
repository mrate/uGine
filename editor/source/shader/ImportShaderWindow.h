#pragma once

#include "../EditorEvents.h"
#include "../EditorTool.h"
#include "../window/Window.h"

#include <ugine/engine/core/Resource.h>
#include <ugine/engine/gfx/Shader.h>

namespace ugine::ed {

class ImportShaderWindow final : public ModalButtonWindow {
public:
    explicit ImportShaderWindow(EditorContext& context);

    const char* Name() const override { return "Import shader"; }
    void Init() override;
    void Show() override;

private:
    struct ShaderError {
        String error;
        Path filename;
        int line{};
        int charPos{};
        String source;
        std::map<std::string, std::string> defines;

        String variantName;
    };

    bool LoadShader(const Path& inputFile, const Path& outputFile, String& importError, Vector<ShaderError>& shaderErrors) const;

    void ImportShader();
    void ShowErrors();
    void ReloadShader(const ResourceID& shader);

    void OnImportResource(const ImportResourceEvent<Shader>& event);
    void OnReloadResource(const ReloadResourceEvent& event);

    EditorContext& context_;

    Path sourcePath_;
    Path targetPath_;

    Vector<ShaderError> shaderErrors_;
};

} // namespace ugine::ed