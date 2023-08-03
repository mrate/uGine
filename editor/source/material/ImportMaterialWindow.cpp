#include "ImportMaterialWindow.h"
#include "MaterialImporter.h"

#include "../platform/FileDialog.h"
#include "../widgets/ImGuiEx.h"
#include "../widgets/ImGuiScope.h"
#include "../widgets/PropertyTable.h"

#include <ugine/Color.h>

#include <glm/glm.hpp>

namespace ugine::ed {

ImportMaterialWindow::ImportMaterialWindow(EditorContext& context)
    : ModalButtonWindow{ context }
    , context_{ context } {
}

void ImportMaterialWindow::Show() {
    if (!materialErrors_.Empty()) {
        ShowErrors();
        return;
    }

    BeginContent();

    bool materialValid{ true };

    {
        PropertyTable table{ "Material settings", &context_ };

        table.ConstProperty("Source:", sourcePath_);

        int id{};
        // TODO:
        //for (auto& param : material_.params) {
        //    ImScope::Id _id(id);

        //    switch (param.type) {
        //        using enum MaterialParamType;

        //    case Float: table.EditProperty(param.name, reinterpret_cast<float&>(material_.paramValues[param.offset])); break;
        //    case Float2: table.EditProperty(param.name, reinterpret_cast<glm::vec2&>(material_.paramValues[param.offset])); break;
        //    case Float3: table.EditProperty(param.name, reinterpret_cast<glm::vec3&>(material_.paramValues[param.offset])); break;
        //    case Float4: table.EditProperty(param.name, reinterpret_cast<glm::vec4&>(material_.paramValues[param.offset])); break;
        //    case Int: table.EditProperty(param.name, reinterpret_cast<int32_t&>(material_.paramValues[param.offset])); break;
        //    case Int2: table.EditProperty(param.name, reinterpret_cast<glm::ivec2&>(material_.paramValues[param.offset])); break;
        //    case Int3: table.EditProperty(param.name, reinterpret_cast<glm::ivec3&>(material_.paramValues[param.offset])); break;
        //    case Int4: table.EditProperty(param.name, reinterpret_cast<glm::ivec4&>(material_.paramValues[param.offset])); break;
        //    case Texture2D:
        //    case Texture3D:
        //    case TextureCube: {
        //        bool found{};
        //        for (auto& [binding, textureId] : material_.textures) {
        //            if (binding != param.binding) {
        //                continue;
        //            }

        //            auto resource{ context_.Engine().GetResources().Get<Texture>(textureId) };
        //            if (table.EditProperty(param.name, resource)) {
        //                textureId = resource->Id();
        //            }
        //            found = true;
        //            break;
        //        }

        //        if (!found) {
        //            materialValid = false;
        //            ResourceHandle<Texture> texture;
        //            if (table.EditProperty(param.name, texture)) {
        //                if (texture) {
        //                    material_.textures.push_back(std::make_pair(param.binding, texture->Id()));
        //                }
        //            }
        //        }

        //        break;
        //    }
        //    default: break;
        //    }
        //}
    }

    context_.DirectorySelector().SelectDirectory("Destination:", targetPath_);
    EndContent();

    SetRightButtonEnabled(0, !targetPath_.Empty() && materialValid);
    Buttons();
}

void ImportMaterialWindow::Init() {
    materialErrors_.Clear();

    const auto files{ OpenFileDialog(context_.Engine().GetPlatform().GetNativeHandle(), MATERIAL_FILES_FILTER, false) };
    if (files.Empty()) {
        context_.Events().CloseModal(this);
        return;
    }

    sourcePath_ = files[0];
    ImportMaterial(sourcePath_);

    SetRightButtons({ Button{
        .enabled = true,
        .name = ICON_FA_FOLDER " Import",
        .func = [this] { CreateMaterial(); },
    } });
}

void ImportMaterialWindow::ImportMaterial(const Path& path) {
    tools::MaterialImporter importer{ path };

    // TODO:
    try {
        material_ = importer.LoadMaterial(context_.ShaderInludeDirs());
    } catch (const std::exception& /*ex*/) {
        for (const auto& error : importer.GetErrors()) {
            materialErrors_.PushBack(MaterialError{
                .error = error.message,
                .filename = error.filename,
            });
        }
    }
}

void ImportMaterialWindow::ShowErrors() {
    SetRightButtons({});
    BeginContent();

    // TODO:

    EndContent();

    Buttons();
}

void ImportMaterialWindow::CreateMaterial() {
    auto [resource, path] = context_.CreateResource<Material>(targetPath_, sourcePath_.Stem());

    Vector<u8> output;
    if (!SaveMaterial(material_, output)) {
        context_.Events().Error("Failed to serialize material.");
        return;
    }

    WriteFileBinary(path, output.ToSpan());

    context_.Events().CloseModal(this);
}

} // namespace ugine::ed