#include "Material.h"

#include "GraphicsState.h"

#include <gfxapi/Initializers.h>

#include <ugine/Log.h>
#include <ugine/Profile.h>

#include <ugine/engine/engine/Engine.h>
#include <ugine/engine/gfx/asset/SerializedMaterial.h>

namespace ugine {

using namespace gfxapi;

glm::mat4 ToMatrix(const MaterialVertexInstance& instance) {
    return glm::mat4{
        glm::vec4{ instance.instance0[0], instance.instance1[0], instance.instance2[0], 0 },
        glm::vec4{ instance.instance0[1], instance.instance1[1], instance.instance2[1], 0 },
        glm::vec4{ instance.instance0[2], instance.instance1[2], instance.instance2[2], 0 },
        glm::vec4{ instance.instance0[3], instance.instance1[3], instance.instance2[3], 1 },
    };
}

MaterialVertexInstance MaterialVertexInstanceFromMatrix(const glm::mat4& matrix) {
    return MaterialVertexInstance{
        .instance0 = glm::vec4{ matrix[0][0], matrix[1][0], matrix[2][0], matrix[3][0] },
        .instance1 = glm::vec4{ matrix[0][1], matrix[1][1], matrix[2][1], matrix[3][1] },
        .instance2 = glm::vec4{ matrix[0][2], matrix[1][2], matrix[2][2], matrix[3][2] },
    };
}

void Material::Prepare(GraphicsState& state, u32 variantMask) {
    if (State() != ResourceState::Loaded) {
        return;
    }

    if (paramsBuffer_.empty()) {
        return;
    }

    const auto pipelineMask{ GetPipeline().VariantMask(variantMask) };
    auto& params{ paramsBuffer_.at(pipelineMask) };

    if (!params.needsUpdate) {
        return;
    }

    params.needsUpdate = false;
    params.index = (params.index + 1) % state.framesInFlight;

    auto ptr{ reinterpret_cast<u8*>(state.device.GetBufferMapped(*params.buffer[params.index])) };

    auto copyUniformValue = [this, ptr](const ShaderParamDescriptor& param, const UniformValue& value) {
        // Overridden value.
        if (value.type == UniformValue::Type::TextureID) {
            auto texture{ Manager().Get<Texture>(value.Get<ResourceID>()) };
            i32 textureId{ BindlessInvalid };
            if (texture) {
                textureId = texture->GetBindlessIndex();
            }
            memcpy(ptr + param.offset, &textureId, param.size);
        } else {
            memcpy(ptr + param.offset, &value.value, param.size);
        }
    };

    const auto& shader{ GetShader()->Variant(pipelineMask) };
    for (auto& param : shader.params) {
        auto value{ params_.find(param.id) };

        // TODO: Simplify.
        if (value == params_.end()) {
            if (IsInstance()) {
                auto originParam{ instanceOrigin_->params_.find(param.id) };
                if (originParam != instanceOrigin_->params_.end()) {
                    copyUniformValue(param, originParam->second);
                    continue;
                }
            }

            // Default shader value.
            const auto defaultValue{ GetShader()->DefaultShaderValue(param.id) };
            if (defaultValue) {
                memcpy(ptr + param.offset, defaultValue->value.data(), defaultValue->Size());
            }
        } else {
            copyUniformValue(param, value->second);
        }
    }
}

void Material::InitShader() {
    UGINE_ASSERT(!shaderInitialized_);
    UGINE_ASSERT(!isInstance_);

    if (!shaderInitialized_) {
        shaderInitialized_ = true;

        InvalidateParams();

        InitPipeline();
        InitParams();
    }
}

void Material::DestroyShader() {
    UGINE_ASSERT(shaderInitialized_);
    UGINE_ASSERT(!isInstance_);

    if (shaderInitialized_) {
        shaderInitialized_ = false;
        DestroyParams();
        DestroyPipeline();
    }
}

void Material::SetShader(ResourceHandle<Shader> shader) {
    UGINE_ASSERT(!isInstance_);

    params_.clear();
    RemoveTextures();

    if (shader_) {
        RemoveDependency(shader_.Get());

        if (shaderInitialized_) {
            DestroyShader();
        }
    }

    UGINE_ASSERT(pipeline_.Empty());

    shaderInitialized_ = false;
    shader_ = shader;

    if (shader) {
        AddDependency(shader_.Get());

        // TODO:
        switch (shader->State()) {
        case ResourceState::Loaded:
            InitShader();

            if (LoadingDependencies() == 0) {
                SetState(ResourceState::Loaded);
            }

            break;
        case ResourceState::Loading: SetState(ResourceState::Loading); break;
        case ResourceState::Failed: SetState(ResourceState::Failed); break;
        case ResourceState::Unloaded: SetState(ResourceState::Unloaded); break;
        }
    } else {
        SetState(ResourceState::Unloaded);
    }
}

void Material::SetMaterialPipeline(const MaterialPipeline& desc) {
    pipelineDesc_ = desc;
    if (shaderInitialized_) {
        DestroyPipeline();
        InitPipeline();
    }
}

bool Material::HandleLoad(Span<const u8> data) {
    PROFILE_EVENT();

    using namespace ugine::gfxapi;

    SerializedMaterial serialized{};
    if (!LoadMaterial(data, serialized)) {
        return false;
    }

    UGINE_DEBUG("Creating material '{}'...", serialized.name);

    pipelineDesc_ = serialized.pipeline;
    isInstance_ = serialized.isInstance;

    UGINE_ASSERT(params_.empty());
    UGINE_ASSERT(textures_.empty());

    for (const auto& [key, val] : serialized.params) {
        auto& param{ params_[StringID{ key }] };
        param = UniformValue{
            .name = StringID{ key },
            .type = UniformValue::Type(val.type),
        };
        memcpy(param.value.data(), val.value.data(), param.Size());

        if (param.type == UniformValue::Type::TextureID) {
            AddTexture(param.Get<ResourceID>());
        }
    }

    if (serialized.isInstance) {
        return InitMaterialInstance(serialized);
    } else {
        return InitMaterial(serialized);
    }
}

void Material::InitPipeline() {
    UGINE_ASSERT(!isInstance_);

    auto state{ Manager().GetEngine().GetState<GraphicsState>() };
    UGINE_ASSERT(state);

    pipeline_.Destroy(*state);
    pipeline_ = Pipeline{ *state, pipelineDesc_, shader_ };
}

void Material::HandleDependencyChanged(const ResourceID& id, ResourceState state) {
    if (shader_ && shader_->Id() == id) {
        switch (state) {
        case ResourceState::Loaded:
            UGINE_ASSERT(!shaderInitialized_);

            if (!shaderInitialized_) {
                InitShader();
            }
            break;
        default:
            if (shaderInitialized_) {
                DestroyShader();
            }
        }
    }

    if (IsInstance() && instanceOrigin_ && instanceOrigin_->Id() == id) {
        switch (state) {
        case ResourceState::Loaded:
            //UGINE_ASSERT(!instanceInitialized_);

            if (!instanceInitialized_) {
                InitInstance(instanceOrigin_);
                InitParams();
            }
            break;
        default:
            if (instanceInitialized_) {
                instanceInitialized_ = false;
                InvalidateParams();
            }
        }
    }
}

void Material::HandleDependenciesReady() {
    if (IsInstance()) {
        if (!instanceInitialized_ && instanceOrigin_) {
            InitInstance(instanceOrigin_);
            InitParams();
        }
    } else {
        if (!shaderInitialized_ && shader_) {
            InitShader();
        }
    }

    InvalidateParams();
}

bool Material::InitMaterial(const SerializedMaterial& serialized) {
    UGINE_ASSERT(!isInstance_);
    UGINE_ASSERT(!serialized.isInstance);

    auto shader{ serialized.shader };
    if (shader.uuid.IsNull()) {
        shader = Manager().FindResource(Path{ serialized.shaderPath.c_str() });
    }

    if (shader.uuid.IsNull()) {
        return false;
    }

    shader_ = Manager().GetEngine().GetResources().Get<Shader>(shader);
    if (!shader_) {
        return false;
    }

    AddDependency(shader_.Get());

    if (shader_->Ready()) {
        InitShader();
    }

    return true;
}

void Material::InitParams() {
    auto state{ Manager().GetEngine().GetState<GraphicsState>() };

    paramsBuffer_.clear();
    for (const auto& [mask, variant] : GetShader()->Variants()) {
        if (variant.uniformSize == 0) {
            continue;
        }

        auto& buffer{ paramsBuffer_[mask] };
        buffer.buffer.Resize(state->framesInFlight);

        for (auto& b : buffer.buffer) {
            b = state->device.CreateBufferUnique(BufferDesc{
                .name = "MaterialBufferUBO",
                .flags = BufferFlags::Uniform,
                .size = variant.uniformSize,
                .cpuAccess = CpuAccessFlags::Write,
            });
        }
    }

    InvalidateParams();
}

bool Material::InitMaterialInstance(const SerializedMaterial& serialized) {
    UGINE_ASSERT(isInstance_);
    UGINE_ASSERT(serialized.isInstance);

    instanceOrigin_ = Manager().Get<Material>(serialized.instanceOf);
    if (!instanceOrigin_) {
        return false;
    }

    AddDependency(instanceOrigin_.Get());
    if (instanceOrigin_->Ready()) {
        InitInstance(instanceOrigin_);
    }

    return true;
}

void Material::InitInstance(ResourceHandle<Material> origin) {
    isInstance_ = true;
    instanceOrigin_ = origin->IsInstance() ? origin->instanceOrigin_ : origin;
    instanceInitialized_ = true;

    InitParams();
}

void Material::InvalidateParams() {
    for (auto& [_, buffer] : paramsBuffer_) {
        buffer.needsUpdate = true;
    }
}

void Material::AddTexture(const ResourceID& id) {
    auto texture{ Manager().Get<Texture>(id) };
    if (texture) {
        AddDependency(texture.Get());
        textures_[texture->Id()] = texture;
    }
}

void Material::RemoveTexture(const ResourceID& id) {
    auto it{ textures_.find(id) };
    if (it != textures_.end()) {
        RemoveDependency(it->second.Get());
        textures_.erase(id);
    }
}

void Material::RemoveTextures() {
    for (auto&& [_, texture] : textures_) {
        RemoveDependency(texture.Get());
    }
    textures_.clear();
}

void Material::DestroyPipeline() {
    UGINE_ASSERT(!isInstance_);

    auto state{ Manager().GetEngine().GetState<GraphicsState>() };
    UGINE_ASSERT(state);

    pipeline_.Destroy(*state);

    UGINE_ASSERT(pipeline_.Empty());
}

void Material::DestroyParams() {
    auto state{ Manager().GetEngine().GetState<GraphicsState>() };
    UGINE_ASSERT(state);

    paramsBuffer_.clear();
}

bool Material::HandleUnload() {
    PROFILE_EVENT();

    if (isInstance_) {
        if (instanceOrigin_) {
            RemoveDependency(instanceOrigin_.Get());
        }

        instanceOrigin_ = {};
    } else {
        SetShader({});
    }

    params_.clear();
    RemoveTextures();

    UGINE_ASSERT(params_.empty());
    UGINE_ASSERT(textures_.empty());

    return true;
}

void Material::Serialize(SerializedMaterial& serialized) const {
    serialized = SerializedMaterial{};

    if (IsInstance()) {
        serialized.isInstance = true;
        serialized.instanceOf = instanceOrigin_->Id();
    } else {
        serialized.isInstance = false;
        serialized.instanceOf = ResourceID{};
        serialized.shader = shader_ ? shader_->Id() : ResourceID{};
        serialized.shaderPath = {};
        serialized.pipeline = pipelineDesc_;
    }

    for (auto&& [name, param] : params_) {
        auto& p{ serialized.params[name] };

        p.type = param.type;
        memcpy(p.value.data(), param.value.data(), param.Size());
    }
}

bool Material::HasVariant(uint32_t variant) const {
    return IsInstance() ? instanceOrigin_->HasVariant(variant) //
        : shader_       ? (shader_->VariantMask() & variant) != 0
                        : false;
}

} // namespace ugine