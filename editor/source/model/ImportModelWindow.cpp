#include "ImportModelWindow.h"

#include "../EditorContext.h"
#include "../platform/FileDialog.h"
#include "../widgets/ImGuiEx.h"
#include "../widgets/ImGuiScope.h"
#include "../widgets/PropertyTable.h"
#include "../window/Window.h"

#include "../material/MaterialCreator.h"

#include <MaterialImporter.h>

#include <ugine/File.h>
#include <ugine/StringUtils.h>
#include <ugine/engine/engine/Engine.h>
#include <ugine/engine/gfx/ImGui.h>
#include <ugine/engine/gfx/Shapes.h>

#include <glm/gtx/transform.hpp>

#include <filesystem>
#include <future>
#include <map>

namespace ugine::ed {

namespace {
    const Vector<String> MESH_FILES_FILTER{ "Mesh files", "*.obj;*.fbx;*.gltf", "All files", "*.*" };
}

ImportModelWindow::ImportModelWindow(EditorContext& context)
    : ModalButtonWindow{ context }
    , context_{ context } {
    settings_.singleMesh = true;
}

void ImportModelWindow::Init() {
    if (mode_ == Mode::ImportModel) {
        const auto files{ OpenFileDialog(context_.Engine().GetPlatform().GetNativeHandle(), MESH_FILES_FILTER, false) };
        if (!files.Empty()) {
            sourcePath_ = files[0];
            meshName_ = sourcePath_.Stem();

            SetStep(0);
        } else {
            Close();
        }
    } else {
        SetStep(0);
    }
}

void ImportModelWindow::Show() {
    if (step_ == 0) {
        switch (mode_) {
        case Mode::ImportModel: ShowImportSettings(); break;
        case Mode::NewShape: ShowNewShapeSettings(); break;
        }
    } else {
        ShowImport();
    }
}

void ImportModelWindow::Close() {
    Clear();
    context_.Events().CloseModal(this);
}

void ImportModelWindow::ShowNewShapeSettings() {
    BeginContent();

    {
        static const std::map<const char*, Shape> Shapes = {
            { "Sphere", Shape::Sphere },
            { "Box", Shape::Box },
            { "Cylinder", Shape::Cylinder },
            { "Capsule", Shape::Capsule },
            { "Plane", Shape::Plane },
        };

        PropertyTable table{ "shape_settings", &context_ };

        table.EditPropertyCombo("Shape", shape_, Shapes);
        table.EditProperty("Material", templateMaterial_);

        SetRightButtonEnabled(0, static_cast<bool>(templateMaterial_));
    }

    EndContent();

    Buttons();
}

void ImportModelWindow::ShowImportSettings() {
    const auto templateReady{ templateMaterial_ && templateMaterial_->Ready() };

    SetRightButtonEnabled(0, !importMeshes_ || importMaterialType_ == ImportMaterial::CreateNew || templateReady);

    BeginContent();

    {
        PropertyTable table{ "import_settings", &context_ };

        table.ConstPropertyUnformatted("File:", sourcePath_.Filename());

        table.EditProperty("Import meshes:", importMeshes_);
        table.EditProperty("Import animations:", importAnimations_);
        table.EditProperty("Fix paths:", settings_.fixPaths);

        if (importMeshes_) {
            ImGui::Separator();

            table.EditProperty("Scale:", scale_);

            //table.BeginProperty("Lod levels:");
            //ImGui::InputInt("##lod", &settings_.lodLevels);
            //table.EndProperty();

            table.EditProperty("Single mesh:", settings_.singleMesh);

            const std::map<const char*, Axis> Axes = {
                { "Y+", Axis::YPos },
                { "Y-", Axis::YNeg },
                { "X+", Axis::XPos },
                { "X-", Axis::XNeg },
                { "Z+", Axis::ZPos },
                { "Z-", Axis::ZNeg },
            };

            const char* axisStr{};
            for (auto [k, v] : Axes) {
                if (v == axisUp_) {
                    axisStr = k;
                    break;
                }
            }

            table.EditPropertyCombo("Axis up:", axisUp_, Axes);
        }
    }

    if (importMeshes_) {
        ImGui::Separator();
        ImportMaterials();
    }

    EndContent();

    Buttons();
}

void ImportModelWindow::ShowImport() {
    BeginContent();

    int id{};

    if (ImGui::BeginTabBar("ImportMesh")) {
        if (ImGui::BeginTabItem("Import")) {
            context_.DirectorySelector().SelectDirectory("Destination:", targetPath_);

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Meshes")) {
            for (const auto& model : models_) {
                ImScope::Id _id{ &id };

                ViewModel(model);
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Tree")) {
            ImScope::Id _id{ &id };
            if (!models_.Empty()) {
                ViewTree(id, 0, 0);
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Animations")) {
            for (const auto& animation : animations_) {
                ImScope::Id _id{ &id };

                ViewAnimation(animation);
            }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    EndContent();

    Buttons();
}

void ImportModelWindow::ViewModel(const SerializedModel& model) {
    ImGui::TextUnformatted("Mesh:");
    ImGui::Text(" - vertices: %u", model.indices.size());
    ImGui::Text(" - triangles: %u", model.indices.size() / 3);
    if (ImGui::TreeNode(" - Meshes")) {
        for (const auto& mesh : model.meshes) {
            ImGui::Text(mesh.name.c_str());
        }
        ImGui::TreePop();
    }

    if (ImGui::TreeNode(" - Bones")) {
        for (const auto& bone : model.bones) {
            ImGui::Text(bone.name.c_str());
        }
        ImGui::TreePop();
    }

    if (ImGui::TreeNode(" - Materials")) {
        for (const auto& material : model.materials) {
            std::stringstream str;
            str << "[";
            for (uint32_t i{}; i < int(SerializedTextureMapping::COUNT); ++i) {
                if (material.images.count(SerializedTextureMapping(i))) {
                    str << ToString(SerializedTextureMapping(i)) << ", ";
                }
            }
            str << "]";

            ImGui::Text("%s %s", material.name.c_str(), str.str().c_str());
        }
        ImGui::TreePop();
    }
}

void ImportModelWindow::ViewAnimation(const SerializedAnimation& animation) {
    ImGui::Text("Animation: '%s'", animation.name.c_str());
    ImGui::Text(" - length: %f s", animation.lengthSeconds);
}

void ImportModelWindow::ViewTree(int& id, uint32_t model, uint32_t i) {
    if (models_[model].nodes.empty()) {
        return;
    }

    ImScope::Id _id{ &id };

    const auto& node{ models_[model].nodes[i] };
    const auto title{ std::format("{} {}", node.name, models_[model].boneNameToIndex.count(node.name) ? ICON_FA_BONE : "") };

    int flags{ i == 0 ? ImGuiTreeNodeFlags_DefaultOpen : 0 };
    if (node.children.empty()) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }

    if (ImGui::TreeNodeEx(title.c_str(), flags)) {
        for (auto child : node.children) {
            ViewTree(id, model, child);
        }
        ImGui::TreePop();
    }
}

void ImportModelWindow::InitMaterialMappings() {
    textureMapping_.clear();
    colorMapping_.clear();
    floatMapping_.clear();

    UGINE_ASSERT(templateMaterial_->Ready());
    if (!templateMaterial_->Ready()) {
        return;
    }

    for (const auto& [name, param] : templateMaterial_->GetShader()->Params()) {
        switch (param.type) {
        case UniformValue::Type::Int:
        case UniformValue::Type::Texture2D:
        case UniformValue::Type::Texture3D:
        case UniformValue::Type::TextureCube:
            {
                const auto lower{ ToLower(param.name.Data()) };
                if (lower.starts_with("diffuse") || lower.starts_with("albedo")) {
                    textureMapping_[param.name] = SerializedTextureMapping::DiffuseMap;
                } else if (lower.starts_with("normal")) {
                    textureMapping_[param.name] = SerializedTextureMapping::NormalMap;
                } else if (lower.starts_with("emissive")) {
                    textureMapping_[param.name] = SerializedTextureMapping::EmissiveMap;
                } else if (lower.starts_with("height")) {
                    textureMapping_[param.name] = SerializedTextureMapping::HeightMap;
                } else if (lower.starts_with("specular")) {
                    textureMapping_[param.name] = SerializedTextureMapping::SpecularMap;
                } else if (lower.starts_with("metalic") || lower.starts_with("roughness")) {
                    textureMapping_[param.name] = SerializedTextureMapping::MetallicRoughnessMap;
                }
            }
            break;
        case UniformValue::Type::Float3:
        case UniformValue::Type::Float4:
            {
                const auto lower{ ToLower(param.name.Data()) };
                if (lower.starts_with("diffuse") || lower.starts_with("albedo")) {
                    colorMapping_[param.name] = SerializedColorMapping::DiffuseColor;
                } else if (lower.starts_with("emissive")) {
                    colorMapping_[param.name] = SerializedColorMapping::EmissiveColor;
                }
            }
            break;
        }
    }
}

void ImportModelWindow::ImportMaterials() {
    static const std::map<const char*, ImportMaterial> importMaterialTypes{
        //{ "Create new", ImportMaterial::CreateNew },
        { "Create instance", ImportMaterial::CreateInstance },
    };

    PropertyTable table{ "Material", &context_ };

    table.EditPropertyCombo("Material", importMaterialType_, importMaterialTypes);

    switch (importMaterialType_) {
    case ImportMaterial::CreateNew: ImportMaterialNew(table); break;
    case ImportMaterial::CreateInstance: ImportMaterialInstance(table); break;
    }
}

void ImportModelWindow::ImportMaterialNew(PropertyTable& table) {
    // TODO: Params?
}

void ImportModelWindow::ImportMaterialInstance(PropertyTable& table) {
    table.BeginProperty("Material template:");

    if (ImGuiEx::SelectResource(context_, templateMaterial_)) {
        initMappings_ = true;
    }

    table.EndProperty();

    if (templateMaterial_ && templateMaterial_->Ready()) {
        if (initMappings_) {
            InitMaterialMappings();
            initMappings_ = false;
        }

        int id{};
        for (const auto& [name, param] : templateMaterial_->GetShader()->Params()) {
            ImScope::Id _id{ &id };

            switch (param.type) {
                using enum UniformValue::Type;

            case Float:
                break;
                table.BeginProperty(param.name);
                if (ImGui::BeginCombo("##vec4_mapping", GetFloatMapping(param.name.Data()))) {
                    for (uint32_t i{}; i < int(SerializedFloatMapping::COUNT); ++i) {
                        if (ImGui::Selectable(ToString(SerializedFloatMapping(i)))) {
                            floatMapping_[param.name] = SerializedFloatMapping(i);
                        }
                    }
                    ImGui::EndCombo();
                }
                table.EndProperty();
                break;
            case Float3:
            case Float4:
                table.BeginProperty(param.name);
                if (ImGui::BeginCombo("##vec4_mapping", GetColorMapping(param.name.Data()))) {
                    for (uint32_t i{}; i < int(SerializedColorMapping::COUNT); ++i) {
                        if (ImGui::Selectable(ToString(SerializedColorMapping(i)))) {
                            colorMapping_[param.name] = SerializedColorMapping(i);
                        }
                    }
                    ImGui::EndCombo();
                }
                table.EndProperty();
                break;
            case Int:
            case TextureID:
                table.BeginProperty(param.name);
                if (ImGui::BeginCombo("##texture_mapping", GetTextureMapping(param.name.Data()))) {
                    for (uint32_t i{}; i < int(SerializedTextureMapping::COUNT); ++i) {
                        if (ImGui::Selectable(ToString(SerializedTextureMapping(i)))) {
                            textureMapping_[param.name] = SerializedTextureMapping(i);
                        }
                    }
                    ImGui::EndCombo();
                }
                table.EndProperty();
                break;
            }
        }
    }
}

void ImportModelWindow::Clear() {
    models_.Clear();
    animations_.Clear();
}

void ImportModelWindow::LoadMesh() {
    switch (importMaterialType_) {
    case ImportMaterial::CreateNew:
        settings_.textureMask = 0;
        settings_.textureMask |= 1 << uint32_t(SerializedTextureMapping::DiffuseMap);
        settings_.textureMask |= 1 << uint32_t(SerializedTextureMapping::EmissiveMap);
        settings_.textureMask |= 1 << uint32_t(SerializedTextureMapping::SpecularMap);
        settings_.textureMask |= 1 << uint32_t(SerializedTextureMapping::NormalMap);
        break;
    case ImportMaterial::CreateInstance:
        settings_.textureMask = 0;
        for (auto [_, mapping] : textureMapping_) {
            settings_.textureMask |= 1 << uint32_t(mapping);
        }
        break;
    };

    settings_.transformation = glm::mat4{ 1.0f };
    switch (axisUp_) {
        using enum Axis;
    case XPos: settings_.transformation *= glm::mat4{ glm ::angleAxis(glm::pi<float>() / 2, glm::vec3{ 0.0f, 0.0f, 1.0f }) }; break;
    case XNeg: settings_.transformation *= glm::mat4{ glm ::angleAxis(glm::pi<float>() / 2, glm::vec3{ 0.0f, 0.0f, -1.0f }) }; break;
    case YPos: break;
    case YNeg: settings_.transformation *= glm::mat4{ glm::angleAxis(glm::pi<float>(), glm::vec3{ 0.0f, 0.0f, 1.0f }) }; break;
    case ZPos: settings_.transformation *= glm::mat4{ glm::angleAxis(glm::pi<float>() / 2, glm::vec3{ -1.0f, 0.0f, 0.0f }) }; break;
    case ZNeg: settings_.transformation *= glm::mat4{ glm::angleAxis(glm::pi<float>() / 2, glm::vec3{ 1.0f, 0.0f, 0.0f }) }; break;
    }
    settings_.transformation *= glm::scale(glm::vec3{ scale_ });

    MeshImporter importer{ sourcePath_, settings_ };
    models_ = importer.LoadMeshes();
    animations_ = importer.LoadAnimations();
}

void ImportModelWindow::GenerateMesh() {
    models_.Clear();
    animations_.Clear();

    auto generateMesh = [this](StringView name, const std::pair<std::vector<MaterialVertex>, std::vector<u16>>& verticesIndices) {
        SerializedModel model{};

        model.aabbMin = glm::vec3(FLT_MAX);
        model.aabbMin = glm::vec3(FLT_MIN);
        for (const auto& v : verticesIndices.first) {
            model.vertices.push_back(SerializedModel::Vertex{
                .pos = v.position,
                .normal = v.normal,
                .tangent = v.tangent,
                .tex = v.uv,
            });

            model.aabbMin = glm::min(model.aabbMin, v.position);
            model.aabbMax = glm::max(model.aabbMax, v.position);
        }
        for (const auto i : verticesIndices.second) {
            model.indices.push_back(i);
        }

        model.meshes.push_back(SerializedModel::Mesh{ name.Data(), glm::mat4{ 1.0f }, 0, 0, u32(model.indices.size()), 0 });
        model.materialIds.push_back(templateMaterial_->Id());
        return model;
    };

    SerializedModel model{};

    switch (shape_) {
    case Shape::Sphere:
        meshName_ = "Sphere";
        model = generateMesh(meshName_, SphereVertices(1, 100, 100));
        break;
    case Shape::Box:
        meshName_ = "Box";
        model = generateMesh(meshName_, CubeVertices(1));
        break;
    case Shape::Cylinder:
        meshName_ = "Cylinder";
        model = generateMesh(meshName_, CubeVertices(1)); // TODO:
        break;
    case Shape::Capsule:
        meshName_ = "Capsule";
        model = generateMesh(meshName_, CubeVertices(1)); /// TODO:
        break;
    case Shape::Plane:
        meshName_ = "Plane";
        model = generateMesh(meshName_, PlaneVertices(1, 1));
        break;
    }

    models_.PushBack(model);
}

void ImportModelWindow::CreateMaterials(SerializedModel& model) {
    auto createMaterial = [&](const SerializedModelMaterial& material) {
        const auto name{ std::format("{}_{}", meshName_.Data(), material.name) };

        std::map<SerializedTextureMapping, ResourceHandle<Texture>> textures;

        MaterialCreator creator{ name };
        for (const auto& [mapping, value] : material.colors) {
            creator.SetColor(mapping, value);
        }

        for (const auto& [mapping, image] : material.images) {
            // Import texture.
            const auto fileName{ std::format("{}_{}_{}", meshName_.Data(), material.name, ToString(mapping)) };
            const auto textureName{ std::format("{}_{}", material.name, ToString(mapping)) };

            // Remember texture ID.
            auto [textureResource, texturePath] = context_.CreateResource<Texture>(targetPath_, fileName.c_str());

            // TODO: Error handling.
            image.Save(texturePath);

            textures[mapping] = textureResource;

            // Add to material.
            creator.AddTexture(mapping);
        }

        // Export generated material to files - one .mat file and couple shader files.
        const auto materialPath{ creator.Build(targetPath_) };

        // Load the generated material from file.
        tools::MaterialImporter importer{ materialPath };
        auto newMaterial{ importer.LoadMaterial(context_.ShaderInludeDirs()) };

        // TODO: Update texture parameters with textures from imported mesh.
        //for (auto& [binding, texture] : newMaterial.textures) {
        //    const auto mapping{ creator.GetMappingForBinding(binding) };
        //    if (mapping) {
        //        texture = textures[*mapping]->Id();
        //    }
        //}

        return newMaterial;
    };

    //std::mutex errorMutex;
    std::vector<std::future<void>> tasks;
    std::vector<std::string> errors;
    std::vector<SerializedMaterial> materials(model.materials.size());

    for (uint32_t index{}; const auto& material : model.materials) {
        //tasks.push_back(std::async([&, index] {
        try {
            materials[index++] = createMaterial(material);
        } catch (const std::exception& ex) {
            // TODO: Null material.

            //std::scoped_lock lock{ errorMutex };
            errors.push_back(ex.what());
        }
        //}));
    }

    //for (auto& task : tasks) {
    //    task.wait();
    //}

    for (const auto& error : errors) {
        UGINE_ERROR("Failed to create new material: {}", error);
    }

    // Finalize new material.
    for (auto& material : materials) {
        auto [newMaterial, path] = context_.CreateResource<Material>(targetPath_, meshName_.Data());

        Vector<u8> data;
        SaveMaterial(material, data);
        WriteFileBinary(path, data.ToSpan());

        model.materialIds.push_back(newMaterial->Id());
    }
}

void ImportModelWindow::CreateMaterialInstances(SerializedModel& model) {
    for (const auto& material : model.materials) {
        std::map<SerializedTextureMapping, ResourceHandle<Texture>> textures;

        auto [newInstance, path] = context_.CreateResource<Material>(targetPath_, std::format("{}_{}", meshName_, material.name).c_str());
        model.materialIds.push_back(newInstance->Id());

        for (const auto& [param, mapping] : textureMapping_) {
            const auto materialImage{ material.images.find(mapping) };
            if (materialImage != material.images.end()) {
                auto texture{ textures.find(mapping) };
                if (texture == textures.end()) {
                    const auto fileName{ std::format("{}_{}_{}", meshName_.Data(), material.name, ToString(mapping)) };
                    const auto textureName{ std::format("{}_{}", material.name, ToString(mapping)) };

                    auto [newTexture, path] = context_.CreateResource<Texture>(targetPath_, fileName.c_str());
                    if (!materialImage->second.Save(path)) {
                        context_.Events().Error("Failed to save texture.");
                    }

                    texture = textures.insert(std::make_pair(mapping, newTexture)).first;
                }

                newInstance->SetTexture(StringID{ param.Data() }, texture->second->Id());
            }
        }

        for (const auto [param, mapping] : colorMapping_) {
            const auto materialColor{ material.colors.find(mapping) };
            if (materialColor != material.colors.end()) {
                newInstance->SetFloat4(StringID{ param.Data() }, materialColor->second);
            }
        }

        for (const auto [param, mapping] : floatMapping_) {
            const auto materialValue{ material.values.find(mapping) };
            if (materialValue != material.values.end()) {
                newInstance->SetFloat(StringID{ param.Data() }, materialValue->second);
            }
        }

        SerializedMaterial serialized{};
        newInstance->Serialize(serialized);
        serialized.isInstance = true;
        serialized.instanceOf = templateMaterial_->Id();

        Vector<u8> out;
        if (!SaveMaterial(serialized, out)) {
            context_.Events().Error("Failed to serialize material instance.");
            continue;
        }

        if (!WriteFileBinary(path, out)) {
            context_.Events().Error("Failed to save material instance.");
            continue;
        }
    }
}

void ImportModelWindow::CreateModels() {
    for (auto& model : models_) {
        switch (importMaterialType_) {
        case ImportMaterial::CreateNew: CreateMaterials(model); break;
        case ImportMaterial::CreateInstance: CreateMaterialInstances(model); break;
        }

        auto [resource, path] = context_.CreateResource<Model>(targetPath_, meshName_.Data());

        Vector<u8> data;
        SaveModel(model, data);
        WriteFileBinary(path, data.ToSpan());
    }
}

void ImportModelWindow::CreateAnimations() {
    for (const auto& animation : animations_) {
        auto [resource, path] = context_.CreateResource<Animation>(targetPath_, std::format("{}_{}", meshName_.Data(), animation.name).c_str());

        Vector<u8> data;
        SaveAnimation(animation, data);
        WriteFileBinary(path, data.ToSpan());
    }
}

void ImportModelWindow::SetStep(int i) {
    step_ = i;

    switch (step_) {
    case 0:
        SetRightButton(Button{
            .enabled = true,
            .name = ICON_FA_ARROW_RIGHT " Continue",
            .func =
                [this] {
                    switch (mode_) {
                    case Mode::ImportModel: LoadMesh(); break;
                    case Mode::NewShape: GenerateMesh(); break;
                    }
                    SetStep(1);
                },
        });
        break;
    case 1:
        SetRightButtons({
            Button{
                .enabled = true,
                .name = ICON_FA_ARROW_LEFT " Back",
                .func =
                    [this] {
                        SetStep(0);
                        models_.Clear();
                        animations_.Clear();
                    },
            },

            Button{
                .enabled = true,
                .name = ICON_FA_FOLDER_OPEN " Import",
                .func =
                    [this] {
                        if (importMeshes_) {
                            CreateModels();
                        }

                        if (importAnimations_) {
                            CreateAnimations();
                        }

                        Close();
                    },
            },
        });
        break;
    }
}

} // namespace ugine::ed