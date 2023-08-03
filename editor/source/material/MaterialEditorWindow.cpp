#include "MaterialEditorWindow.h"
#include "MaterialTool.h"

#include <ugine/Profile.h>
#include <ugine/engine/core/ResourceManager.h>
#include <ugine/engine/gfx/ImGui.h>
#include <ugine/engine/gfx/asset/SerializedMaterial.h>

namespace ugine::ed {

namespace {
    const std::map<const char*, MaterialBlending> BlendingCombo = {
        { "Opaque", MaterialBlending::Opaque },
        { "Masked", MaterialBlending::Masked },
        { "Additive", MaterialBlending::Additive },
        { "Translucent", MaterialBlending::Translucent },
    };
}

template <typename T, typename... Args>
bool PopulateMaterialParam(
    EditorContext& context, PropertyTable& table, Material* material, const String& name, StringID id, UniformValue::Type type, Args&&... args) {
    bool updated{};
    auto val{ material->GetParam<T>(id) };
    const auto hasVal{ material->HasParam(id) };

    table.BeginProperty(name.Data());
    {
        ImScope::ItemWidth itemWidth{ -1 };

        if (table.GuiWidget(val, std::forward<Args>(args)...)) {
            material->SetParam(id, type, val);
            updated = true;
        }
    }

    ImGui::TableSetColumnIndex(2);

    {
        ImScope::Disabled disabled{ !hasVal };

        ImGui::TextUnformatted(ICON_FA_UNDO);
        if (ImGui::IsItemClicked(0)) {
            material->ResetParam(id);
            updated = true;
        }
    }

    table.EndProperty();

    return updated;
}

template <typename... Args>
bool PopulateMaterialParamTexture(EditorContext& context, PropertyTable& table, Material* material, const String& name, const StringID& id, Args&&... args) {
    bool updated{};
    auto val{ material->GetParam<ResourceID>(id) };
    const auto hasVal{ material->HasParam(id) };
    auto resource{ context.Engine().GetResources().Get<Texture>(val) };

    table.BeginProperty(name.Data());

    {
        ImScope::ItemWidth itemWidth{ -1 };

        if (table.GuiWidget(resource, std::forward<Args>(args)...)) {
            if (resource) {
                material->SetParam(id, UniformValue::Type::TextureID, resource->Id());
            } else {
                material->SetParam(id, UniformValue::Type::TextureID, ResourceID{});
            }
            updated = true;
        }
    }

    ImGui::TableSetColumnIndex(2);

    {
        ImScope::Disabled disabled{ !hasVal };
        ImGui::TextUnformatted(ICON_FA_UNDO);
        if (ImGui::IsItemClicked(0)) {
            material->ResetParam(id);
            updated = true;
        }
    }

    table.EndProperty();

    return updated;
}

MaterialEditorWindow::MaterialEditorWindow(EditorContext& context, StringView id)
    : EditorWindow{ context, false, MATERIAL_RESOURCE_ICON " Material", id, ImGuiWindowFlags_NoSavedSettings }
    , preview_{ context } {
}

void MaterialEditorWindow::SetMaterial(ResourceHandle<Material> material) {
    // TODO: if (modified_) ...
    material_ = material;
    preview_.SetMaterial(material_);

    if (material_) {
        visible_ = true;
    }
}

bool MaterialEditorWindow::Valid() const {
    return static_cast<bool>(material_);
}

void MaterialEditorWindow::Content() {
    if (!material_) {
        preview_.SetRendering(false);
        return;
    }

    PROFILE_EVENT_NC("MaterialEditorWindow", COLOR_EDITOR_PROFILE);
    preview_.SetRendering(true);

    try {
        RenderMaterial();
    } catch (const std::exception& ex) {
        context_.Events().Error(ex.what());
    }
}

void MaterialEditorWindow::Reload() {
    if (material_) {
        context_.Engine().GetResources().Reload<Material>(material_->Id());
    }
}

bool MaterialEditorWindow::Save() {
    const auto path{ context_.Engine().GetResources().ResourcePath(material_->Id()) };
    context_.Events().ResetThumbnail(material_->Id());

    UGINE_DEBUG("Saving material '{}'...", path.String());

    SerializedMaterial serialized{};
    material_->Serialize(serialized);
    Vector<u8> out;
    if (!SaveMaterial(serialized, out)) {
        return false;
    }

    return WriteFileBinary(path, out.ToSpan());
}

void MaterialEditorWindow::Closed() {
    preview_.SetRendering(false);
    SetMaterial({});
}

void MaterialEditorWindow::OnOpenTypelessResource(const OpenTypelessResourceEvent& event) {
    if (event.resource.type == Material::TYPE) {
        SetMaterial(context_.Engine().GetResources().Get<Material>(event.resource.id));
    }
}
//
//void MaterialEditorWindow::CreateNewMaterial() {
//    auto [material, path] = context_.CreateResource<Material>(context_.ProjectDirectory() / "new", "new_material");
//
//    SerializedMaterial empty{};
//    Vector<u8> data;
//    if (!SaveMaterial(empty, data)) {
//        context_.Events().Error("Failed to serialize material.");
//        return;
//    }
//
//    if (!WriteFileBinary(path, data.ToSpan())) {
//        context_.Events().Error("Failed to write file.");
//    }
//}

void MaterialEditorWindow::Populate() {
    ImScope::Indent ident;

    PropertyTable table{ "Material", &context_, 3, false };
    ImGui::TableSetupColumn("##name", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("##property", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("##default", ImGuiTableColumnFlags_WidthFixed);

    auto pipeline{ material_->GetMaterialPipeline() };
    auto shader{ material_->GetShader() };

    if (material_->IsInstance()) {
        const auto originPath{ context_.Engine().GetResources().ResourcePath(Material::InstanceOrigin(material_)->Id()).Filename() };

        table.ConstPropertyUnformatted("Material", originPath);
    } else {
        const auto name{ context_.Engine().GetResources().ResourcePath(material_->Id()).Filename() };

        table.ConstPropertyUnformatted("Name", name);

        if (table.EditProperty("Shader", shader)) {
            material_->SetShader(shader);

            SetModified(true);
        }

        auto changed = table.EditProperty("Double sided", pipeline.doubleSided);
        changed = table.EditProperty("Depth write", pipeline.depthWrite) || changed;
        changed = table.EditProperty("Depth test", pipeline.depthTest) || changed;
        changed = table.EditProperty("Wireframe", pipeline.wireframe) || changed;
        changed = table.EditPropertyCombo("Blending", pipeline.blending, BlendingCombo) || changed;

        if (changed) {
            material_->SetMaterialPipeline(pipeline);

            SetModified(true);
        }
    }

    if (shader) {
        ImGui::Separator();
        table.ConstProperty("Params", StringView{});

        for (const auto& [name, param] : shader->Params()) {
            ImScope::Id id(name.Data());

            const auto updated{ [&] {
                switch (param.type) {
                    using enum UniformValue::Type;

                case Float: return PopulateMaterialParam<float>(context_, table, material_.Get(), param.name, param.id, UniformValue::Type::Float);
                case Float2: return PopulateMaterialParam<glm::vec2>(context_, table, material_.Get(), param.name, param.id, UniformValue::Type::Float2);
                case Float3: return PopulateMaterialParam<glm::vec3>(context_, table, material_.Get(), param.name, param.id, UniformValue::Type::Float3, true);
                case Float4: return PopulateMaterialParam<glm::vec4>(context_, table, material_.Get(), param.name, param.id, UniformValue::Type::Float4, true);
                case Int:
                    {
                        // TODO: Make switchable
                        const auto texture{ PopulateMaterialParamTexture(context_, table, material_.Get(), param.name, param.id) };
                        //const auto value{ PopulateMaterialParam<i32>(context_, table, material_.Get(), param.name, param.id, UniformValue::Type::Int) };
                        return texture /*|| value*/;
                    }
                case Int2: return PopulateMaterialParam<glm::ivec2>(context_, table, material_.Get(), param.name, param.id, UniformValue::Type::Int2);
                case Int3: return PopulateMaterialParam<glm::ivec3>(context_, table, material_.Get(), param.name, param.id, UniformValue::Type::Int3);
                case Int4: return PopulateMaterialParam<glm::ivec4>(context_, table, material_.Get(), param.name, param.id, UniformValue::Type::Int4);
                default: return false;
                }
            }() };

            if (updated) {
                SetModified(true);
            }
        }
    }
}

void MaterialEditorWindow::RenderMaterial() {
    {
        ImScope::Disabled disabled{ material_->IsInstance() || material_->State() != ResourceState::Loaded };

        if (ImGui::Button(ICON_FA_CONTENT_COPY " Create instance")) {
            auto targetPath{ context_.Engine().GetResources().ResourcePath(material_->Id()) };
            if (!targetPath.Empty()) {
                targetPath = targetPath.ParentPath();
            }

            const auto name{ "MaterialInstance" };
            auto [resource, path] = context_.CreateResource<Material>(targetPath, name);

            SerializedMaterial serialized{};
            serialized.name = name;
            serialized.instanceOf = Material::InstanceOrigin(material_)->Id();
            serialized.isInstance = true;

            Vector<u8> out;
            if (!SaveMaterial(serialized, out)) {
                context_.DeleteResource(resource);
                context_.Events().Error("Error serializing material instance.");
                return;
            }

            if (!WriteFileBinary(path, out)) {
                context_.DeleteResource(resource);
                context_.Events().Error("Error writing material instance file.");
                return;
            }
        }
    }

    const auto flags{ ImGuiTableFlags_Resizable };
    ImGui::BeginTable("##MaterialEditor", 2, flags);
    ImGui::TableSetupColumn("##preview", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);

    switch (material_->State()) {
    case ResourceState::Loaded: RenderPreview(); break;
    case ResourceState::Loading: ImGuiEx::Loading("Loading", 256.0f); break;
    default: ImGui::TextUnformatted("Material error."); break;
    }

    ImGui::TableSetColumnIndex(1);

    Populate();

    ImGui::EndTable();
}

void MaterialEditorWindow::RenderPreview() {
    if (ImGui::BeginChild("Preview")) {
        auto pos{ ImGui::GetCursorPos() };
        pos.x += 10;
        pos.y += 10;

        const auto size{ ImGui::GetContentRegionAvail() };
        preview_.Resize(u32(size.x), u32(size.y));
        preview_.Render();

        ImGui::SetCursorPos(pos);

        const auto w{ ImGui::GetFontSize() * 2 - 5 };
        const auto h{ ImGui::GetFontSize() * 2 - 5 };
        const auto padding{ ImGui::GetStyle().FramePadding };
        const auto spacing{ ImGui::GetStyle().ItemSpacing.x };

        if (ImGuiEx::BeginToolbar(2, ImVec2{ 4 * w + 3 * spacing + 2 * padding.x, h + 2 * padding.y })) {
            const ImVec4 selectedColor{ 1, 1, 1, 1 };
            const ImVec4 defaultColor{ 0.5f, 0.5f, 0.5f, 1 };

            auto button = [&](const char* icon, auto model) {
                ImScope::Color text{ ImGuiCol_Text, preview_.PreviewModel() == model ? selectedColor : defaultColor };

                if (ImGui::Button(icon, ImVec2{ w, h })) {
                    preview_.SetPreviewModel(model);
                }
            };

            ImScope::StyleVar padding{ ImGuiStyleVar_FramePadding, ImVec2{ 2, 2 } };
            ImScope::StyleVar rounding{ ImGuiStyleVar_FrameRounding, 0.0f };

            button(ICON_FA_CIRCLE, WorldPreviewRenderer::MaterialPreview::Sphere);
            ImGui::SameLine();
            button(ICON_FA_CUBE, WorldPreviewRenderer::MaterialPreview::Cube);
            ImGui::SameLine();
            button(ICON_FA_SQUARE, WorldPreviewRenderer::MaterialPreview::Plane);
            ImGui::SameLine();
            button(ICON_FA_CLOUD, WorldPreviewRenderer::MaterialPreview::Skybox);
        }
        ImGuiEx::EndToolbar();
    }

    ImGui::EndChild();
}

} // namespace ugine::ed