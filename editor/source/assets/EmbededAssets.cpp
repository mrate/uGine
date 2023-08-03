#include "EmbededAssets.h"

#include "../EditorContext.h"

#include <ugine/File.h>
#include <ugine/Path.h>

#include <ugine/engine/gfx/Shapes.h>
#include <ugine/engine/gfx/asset/SerializedAnimation.h>
#include <ugine/engine/gfx/asset/SerializedMaterial.h>
#include <ugine/engine/gfx/asset/SerializedModel.h>

// TODO:
#include "../material/MaterialTool.h"

// Generated files.
#include <ugineTools/Embed.h>

#include <bone_material.h>
#include <grid_material.h>

#include <bone_shader.h>
#include <grid_shader.h>

namespace ugine::ed {

EmbededAssets::EmbededAssets(EditorContext& context)
    : context_{ context } {
    auto state{ context_.Engine().GetState<GraphicsState>() };

    auto& resources{ context.Engine().GetResources() };

    ResourceHandle<Shader> gridShader;
    ResourceHandle<Shader> boneShader;

    {
        gridShader = context_.Engine().GetResources().Create<Shader>();
        resources.RegisterResource(gridShader->Id(), Path{ "grid.ush" }, gridShader->Type());
        gridShader->Load(Span{ reinterpret_cast<const u8*>(grid_shader.data), grid_shader.size });
    }

    {
        boneShader = resources.Create<Shader>();
        resources.RegisterResource(boneShader->Id(), Path{ "bone.ush" }, boneShader->Type());
        boneShader->Load(Span{ reinterpret_cast<const u8*>(bone_shader.data), bone_shader.size });
    }

    auto gridMaterial{ resources.Create<Material>() };
    {
        resources.RegisterResource(gridMaterial->Id(), Path{ "grid.umat" }, gridMaterial->Type());
        gridMaterial->Load(Span{ reinterpret_cast<const u8*>(grid_material.data), grid_material.size });

        SerializedModel gridModel{};

        constexpr auto gridSize{ 10 };
        for (int x{ -gridSize }; x <= gridSize; ++x) {
            gridModel.vertices.emplace_back(SerializedModel::Vertex{ { x, 0, -gridSize } });
            gridModel.vertices.emplace_back(SerializedModel::Vertex{ { x, 0, gridSize } });
            gridModel.indices.emplace_back(uint16_t(gridModel.vertices.size() - 2));
            gridModel.indices.emplace_back(uint16_t(gridModel.vertices.size() - 1));

            gridModel.vertices.emplace_back(SerializedModel::Vertex{ { -gridSize, 0, x } });
            gridModel.vertices.emplace_back(SerializedModel::Vertex{ { gridSize, 0, x } });
            gridModel.indices.emplace_back(uint16_t(gridModel.vertices.size() - 2));
            gridModel.indices.emplace_back(uint16_t(gridModel.vertices.size() - 1));
        }

        gridModel.meshes.push_back(SerializedModel::Mesh{ "Grid", glm::mat4{ 1.0f }, 0, 0, u32(gridModel.indices.size()), 0 });
        gridModel.aabbMin = glm::vec3{ -gridSize, 0, -gridSize };
        gridModel.aabbMax = glm::vec3{ gridSize, 0, gridSize };

        gridModel.materialIds.push_back(gridMaterial->Id());

        Vector<u8> out;
        SaveModel(gridModel, out);

        GridModel = resources.Create<Model>();
        resources.RegisterResource(GridModel->Id(), Path{ "grid.umod" }, GridModel->Type());
        GridModel->Load(out.ToSpan());
    }

    {
        BoneMaterial = resources.Create<Material>();
        resources.RegisterResource(BoneMaterial->Id(), Path{ "bone.umat" }, BoneMaterial->Type());
        BoneMaterial->Load(Span{ reinterpret_cast<const u8*>(bone_material.data), bone_material.size });

        Vector<u8> buffer;
        // TODO:
        SaveMaterial(InstantiateMaterial(BoneMaterial, "Selected bone"), buffer);
        SelectedBoneMaterial = resources.Create<Material>();
        resources.RegisterResource(SelectedBoneMaterial->Id(), Path{ "selected_bone.umat" }, SelectedBoneMaterial->Type());
        SelectedBoneMaterial->Load(buffer.ToSpan());
        SelectedBoneMaterial->SetFloat4("materialColor"_hs, glm::vec4{ 0.9f, 0.4f, 0.4f, 0.9f });

        const std::vector<SerializedModel::Vertex> vertices = {
            { { 0.0f, 0.0f, -0.5f } },
            { { 0.075f, 0.0f, 0.5f } },
            { { 0.0f, 0.075f, 0.5f } },
            { { -0.075f, 0.0f, 0.5f } },
            { { 0.0f, -0.075f, 0.5f } },
        };

        const std::vector<u32> indices = { 0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 1 };

        SerializedModel boneModel{};
        boneModel.vertices = vertices;
        boneModel.indices = indices;
        boneModel.meshes.push_back(SerializedModel::Mesh{ "Bone", glm::mat4{ 1.0f }, 0, 0, u32(indices.size()), 0 });
        boneModel.aabbMin = glm::vec3{ -0.075f, -0.075f, -0.5f };
        boneModel.aabbMax = glm::vec3{ -1.0f } * boneModel.aabbMin;

        boneModel.materialIds.push_back(BoneMaterial->Id());

        {
            Vector<u8> out;
            SaveModel(boneModel, out);

            BoneModel = resources.Create<Model>();
            resources.RegisterResource(BoneModel->Id(), Path{ "bone.umodel" }, BoneModel->Type());
            BoneModel->Load(out.ToSpan());
        }
    }

    {
        SerializedAnimation animation{};
        animation.name = "Empty";

        Vector<u8> out;
        SaveAnimation(animation, out);

        EmptyAnimation = resources.Create<Animation>();
        resources.RegisterResource(EmptyAnimation->Id(), Path{ "empty.uanim" }, EmptyAnimation->Type());
        EmptyAnimation->Load(out.ToSpan());
    }

    {
        auto loadTexture = [&](StringView file) {
            auto res{ resources.Create<Texture>() };

            const auto data{ ReadFileBinary(context_.EditorPath() / "editor" / file.Data()) };
            if (data.Empty()) {
                UGINE_ERROR("Failed to read editor texture {}", file.Data());
                return res;
            }

            res->Load(data.ToSpan());
            if (!res->Ready()) {
                UGINE_ERROR("Failed to read editor/file.png");
            }

            return res;
        };

        FileIcon = loadTexture("file.png");
        WorldIcon = loadTexture("world.png");
        AnimationIcon = loadTexture("animation.png");
        ScriptIcon = loadTexture("script.png");
        ShaderIcon = loadTexture("shader.png");
    }
}

EmbededAssets::~EmbededAssets() {
}

ResourceHandle<Model> EmbededAssets::CreateSphere() {
    const auto [sphereVertices, sphereIndices]{ SphereVertices(1, 20, 20) };

    SerializedModel sphereModel{};
    for (const auto& vertex : sphereVertices) {
        sphereModel.vertices.push_back(SerializedModel::Vertex{ vertex.position, vertex.normal, vertex.tangent, vertex.uv });
    }
    for (auto index : sphereIndices) {
        sphereModel.indices.push_back(index);
    }
    sphereModel.meshes.push_back(SerializedModel::Mesh{ "Sphere", glm::mat4{ 1.0f }, 0, 0, u32(sphereIndices.size()), 0 });
    sphereModel.materialIds.push_back(ResourceID{});

    Vector<u8> out(1024 * 1024);
    SaveModel(sphereModel, out);

    auto model{ context_.Engine().GetResources().Create<Model>() };
    model->Load(out.ToSpan());
    return model;
}

ResourceHandle<Model> EmbededAssets::CreateCube() {
    const auto [vertices, indices]{ CubeVertices(0.5f) };

    SerializedModel model{};
    for (const auto& vertex : vertices) {
        model.vertices.push_back(SerializedModel::Vertex{ vertex.position, vertex.normal, vertex.tangent, vertex.uv });
    }
    for (auto index : indices) {
        model.indices.push_back(index);
    }
    model.meshes.push_back(SerializedModel::Mesh{ "Cube", glm::mat4{ 1.0f }, 0, 0, u32(indices.size()), 0 });
    model.materialIds.push_back(ResourceID{});

    Vector<u8> out(1024 * 1024);
    SaveModel(model, out);

    auto res{ context_.Engine().GetResources().Create<Model>() };
    res->Load(out.ToSpan());
    return res;
}

ResourceHandle<Model> EmbededAssets::CreatePlane() {
    const auto [vertices, indices]{ PlaneVertices(0.5f, 0.5f) };

    SerializedModel model{};
    for (const auto& vertex : vertices) {
        model.vertices.push_back(SerializedModel::Vertex{ vertex.position, vertex.normal, vertex.tangent, vertex.uv });
    }
    for (auto index : indices) {
        model.indices.push_back(index);
    }
    model.meshes.push_back(SerializedModel::Mesh{ "Cube", glm::mat4{ 1.0f }, 0, 0, u32(indices.size()), 0 });
    model.materialIds.push_back(ResourceID{});

    Vector<u8> out(1024 * 1024);
    SaveModel(model, out);

    auto res{ context_.Engine().GetResources().Create<Model>() };
    res->Load(out.ToSpan());
    return res;
}

ResourceHandle<Model> EmbededAssets::CreateSkybox() {
    const auto [vertices, indices]{ CubeVertices(100) };

    SerializedModel model{};
    for (const auto& vertex : vertices) {
        model.vertices.push_back(SerializedModel::Vertex{ vertex.position, vertex.normal, vertex.tangent, vertex.uv });
    }

    for (auto index : indices) {
        model.indices.push_back(index);
    }

    // Invert winding.
    for (int i{}; i < model.indices.size(); i += 3) {
        std::swap(model.indices[i], model.indices[i + 2]);
    }

    model.meshes.push_back(SerializedModel::Mesh{ "Cube", glm::mat4{ 1.0f }, 0, 0, u32(indices.size()), 0 });
    model.materialIds.push_back(ResourceID{});

    Vector<u8> out(1024 * 1024);
    SaveModel(model, out);

    auto res{ context_.Engine().GetResources().Create<Model>() };
    res->Load(out.ToSpan());
    return res;
}

} // namespace ugine::ed