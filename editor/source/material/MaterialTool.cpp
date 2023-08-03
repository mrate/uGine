#include "MaterialTool.h"

#include "../EditorContext.h"
#include "../EditorTool.h"

#include "ImportMaterialWindow.h"
#include "MaterialEditorWindow.h"
#include "MaterialInstanceWindow.h"

#include <ugine/Memory.h>

namespace ugine::ed {

// TODO: Move to Material.h
SerializedMaterial InstantiateMaterial(ResourceHandle<Material> material, StringView name) {
    SerializedMaterial serialized{};

    material->Serialize(serialized);

    serialized.name = name.Data();
    serialized.isInstance = true;
    serialized.instanceOf = Material::InstanceOrigin(material)->Id();
    serialized.params.clear();

    return serialized;
}

class MaterialTool : public EditorTool {
public:
    MaterialTool(EditorContext& context)
        : EditorTool{ context }
        , importMaterial_{ context } {
        auto& fileMenu{ context_.MainMenu().Get("File") };
        fileMenu.AddAction(MATERIAL_RESOURCE_ICON " New material", [this] { CreateNewMaterial(); });
        fileMenu.AddSeparator();

        auto& importMenu{ context_.MainMenu().Get("Import") };
        importMenu.AddAction(MATERIAL_RESOURCE_ICON " Material...", [this] { context_.Events().ShowModal(&importMaterial_); });

        context_.Events().Connect<OpenResourceEvent<Material>, &MaterialTool::OnOpenMaterial>(this);
        context_.Events().Connect<OpenTypelessResourceEvent, &MaterialTool::OnOpenTypelessResource>(this);
    }

    void Render() {
        for (auto& editor : materialEditors_) {
            editor->Render();
        }
    }

    void CreateNewMaterial() {
        auto [material, path] = context_.CreateResource<Material>(context_.SelectedAssetPath(), "new_material");

        SerializedMaterial empty{};
        Vector<u8> data;
        if (!SaveMaterial(empty, data)) {
            context_.Events().Error("Failed to serialize material.");
            return;
        }

        if (!WriteFileBinary(path, data.ToSpan())) {
            context_.Events().Error("Failed to write file.");
        }
    }

    void OnOpenTypelessResource(const OpenTypelessResourceEvent& event) {
        if (event.resource.type == Material::TYPE) {
            OpenMaterial(context_.Engine().GetResources().Get<Material>(event.resource.id));
        }
    }

    void OnOpenMaterial(const OpenResourceEvent<Material> event) { OpenMaterial(event.resource); }

    void OpenMaterial(ResourceHandle<Material> material) {
        auto editor{ MakeUnique<MaterialEditorWindow>(context_.Allocator(), context_, std::to_string(cnt_++).c_str()) };
        editor->SetMaterial(material);

        materialEditors_.PushBack(std::move(editor));
    }

private:
    int cnt_{};
    Vector<UniquePtr<MaterialEditorWindow>> materialEditors_;
    ImportMaterialWindow importMaterial_;
};

namespace {
    EditorToolset::Register _{ [](EditorContext& context) {
        context.RegisterResourceType<Material>(MATERIAL_RESOURCE_ICON, ".umat", ColorRGB{ 1, 0.75f, 0.75f });

        context.RegisterEditorTool(MakeUnique<MaterialTool>(context.Allocator(), context));
    } };
} // namespace

} // namespace ugine::ed