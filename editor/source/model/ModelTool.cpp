#include "ModelTool.h"

#include "../EditorContext.h"
#include "../EditorTool.h"

#include "ImportModelWindow.h"
#include "ModelEditorWindow.h"

#include <ugine/Memory.h>

namespace ugine::ed {

class ModelTool : public EditorTool {
public:
    explicit ModelTool(EditorContext& context)
        : EditorTool{ context }
        , importModel_{ context } {
        auto& importMenu{ context_.MainMenu().Get("Import") };
        importMenu.AddAction(MODEL_RESOURCE_ICON " Mesh...", [this] {
            importModel_.SetMode(ImportModelWindow::Mode::ImportModel);
            context_.Events().ShowModal(&importModel_);
        });

        auto& fileMenu{ context_.MainMenu().Get("File") };
        fileMenu.AddAction(MODEL_RESOURCE_ICON " Shape model...", [this] {
            importModel_.SetMode(ImportModelWindow::Mode::NewShape);
            context_.Events().ShowModal(&importModel_);
        });
    }

    void Render() override {
        // TODO:
    }

private:
    ImportModelWindow importModel_;
};

namespace {
    EditorToolset::Register _{ [](EditorContext& context) {
        context.RegisterResourceType<Model>(MODEL_RESOURCE_ICON, ".umodel", ColorRGB{ 0.75f, 1, 0.75f });

        context.RegisterEditorTool(MakeUnique<ModelTool>(context.Allocator(), context));
        context.RegisterEditorTool(MakeUnique<ModelEditorWindow>(context.Allocator(), context));
    } };
} // namespace

} // namespace ugine::ed