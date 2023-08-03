#pragma once

#include "../window/Window.h"

#include "MeshImporter.h"

#include <ugine/engine/gfx/asset/SerializedAnimation.h>
#include <ugine/engine/gfx/asset/SerializedMaterial.h>
#include <ugine/engine/gfx/asset/SerializedModel.h>

#include <map>

namespace ugine::ed {

class EditorContext;
struct PropertyTable;

class ImportModelWindow : public ModalButtonWindow {
public:
    enum class Mode {
        ImportModel,
        NewShape,
    };

    explicit ImportModelWindow(EditorContext& context);

    void SetMode(Mode mode) { mode_ = mode; }

    const char* Name() const override { return "Import mesh"; }
    void Init() override;
    void Show() override;

private:
    void Close();

    enum class ImportMaterial {
        CreateNew,
        CreateInstance,
    };

    enum class Shape {
        Sphere,
        Box,
        Cylinder,
        Capsule,
        Plane,
    };

    static const char* ToString(SerializedTextureMapping mapping) {
        static const char* ToString[] = {
            "Diffuse Map",
            "Emissive Map",
            "Normal Map",
            "Height Map",
            "Ambient Map",
            "Specular Map",
            "Ao Map",
            "MetallicRoughness Map",
        };
        return ToString[int(mapping)];
    }

    static const char* ToString(SerializedColorMapping mapping) {
        static const char* ToString[] = {
            "Ambient Color",
            "Diffuse Color",
            "Specular Color",
            "Emissive Color",
            "BaseColor Factor",
        };
        return ToString[int(mapping)];
    }

    static const char* ToString(SerializedFloatMapping mapping) {
        static const char* ToString[] = {
            "Shininess",
            "Metallic Factor",
            "Roughness Factor",
        };
        return ToString[int(mapping)];
    }

    const char* GetTextureMapping(StringView name) {
        const auto it{ textureMapping_.find(name) };
        return it == textureMapping_.end() ? "" : ToString(it->second);
    }

    const char* GetColorMapping(StringView name) {
        const auto it{ colorMapping_.find(name) };
        return it == colorMapping_.end() ? "" : ToString(it->second);
    }

    const char* GetFloatMapping(StringView name) {
        const auto it{ floatMapping_.find(name) };
        return it == floatMapping_.end() ? "" : ToString(it->second);
    }

    void ShowImportSettings();
    void ShowNewShapeSettings();
    void ShowImport();
    void ViewModel(const SerializedModel& model);
    void ViewAnimation(const SerializedAnimation& animation);
    void ViewTree(int& id, uint32_t model, uint32_t i);

    void ImportMaterials();
    void ImportMaterialNew(PropertyTable& table);
    void ImportMaterialInstance(PropertyTable& table);

    void InitMaterialMappings();

    void Clear();

    void LoadMesh();
    void GenerateMesh();

    void CreateMaterials(SerializedModel& model);
    void CreateMaterialInstances(SerializedModel& model);
    void CreateModels();
    void CreateAnimations();

    void SetStep(int i);

    EditorContext& context_;

    Path sourcePath_{};
    Path targetPath_{};

    MeshImporter::Settings settings_{};

    String meshName_;

    Mode mode_{ Mode::ImportModel };

    float scale_{ 1.0f };
    Axis axisUp_{ Axis::YPos };

    Shape shape_{ Shape::Sphere };

    bool importMeshes_{ true };
    bool importAnimations_{ true };

    int step_{};
    bool initMappings_{};

    Vector<SerializedModel> models_;
    Vector<SerializedAnimation> animations_;

    // Material import.
    ImportMaterial importMaterialType_{ ImportMaterial::CreateInstance };

    ResourceHandle<Material> templateMaterial_;
    std::map<String, SerializedTextureMapping> textureMapping_;
    std::map<String, SerializedColorMapping> colorMapping_;
    std::map<String, SerializedFloatMapping> floatMapping_;
};

} // namespace ugine::ed