#include "WorldSerializer.h"
#include "World.h"

#include <ugine/engine/core/Json.h>
#include <ugine/engine/engine/Engine.h>
#include <ugine/engine/gfx/Component.h>
#include <ugine/engine/script/Component.h>

#include <ugine/Json.h>
#include <ugine/Path.h>
#include <ugine/Stream.h>
#include <ugine/String.h>

#include <fstream>

namespace ugine {

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Transformation, position, rotation, scale);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(LuaScriptComponent, script);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MaterialVertex, position, normal, tangent, uv);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MaterialVertexInstance, instance0, instance1, instance2);

struct SerializationContext {
    World& world;
    ResourceManager& resources;
    std::map<GameObjectHandle, GameObjectHandle> idMap;
    Vector<GameObjectHandle> deserialized;
};

#define JSON_DEFAULT_VAL()                                                                                                                                     \
    std::decay_t<decltype(comp)> jsonDefaultVal {}
#define TO_JSON(MEMBER) json[#MEMBER] = comp.MEMBER;
#define FROM_JSON(MEMBER) comp.MEMBER = json.value(#MEMBER, jsonDefaultVal.MEMBER);

#define COMPONENT(TYPE, ...)                                                                                                                                   \
    void Serialize(nlohmann::json& json, const TYPE& comp) { NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(TO_JSON, __VA_ARGS__)) }                                 \
    void Deserialize(const nlohmann::json& json, SerializationContext& context, TYPE& comp) {                                                                  \
        JSON_DEFAULT_VAL();                                                                                                                                    \
        NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(FROM_JSON, __VA_ARGS__))                                                                                      \
    }

template <typename T> nlohmann::json SerializeResource(const ResourceHandle<T>& handle) {
    return handle.Get() ? handle->Id() : ResourceID{};
}

template <typename T> ResourceHandle<T> DeserializeResource(const nlohmann::json& js, StringView name, ResourceManager& resources) {
    auto id{ [&]() -> ResourceID {
        try {
            return js.value<ResourceID>(name.Data(), ResourceID{});
        } catch (...) {
            return ResourceID{};
        }
    }() };

    return id.IsNull() ? ResourceHandle<T>{} : resources.Get<T>(id);
}

template <typename T> nlohmann::json SerializeResources(Span<const ResourceHandle<T>> resources) {
    std::vector<ResourceID> resourceIds(resources.Size());
    for (u32 i{}; auto& resource : resources) {
        resourceIds[i] = resources[i] ? resources[i]->Id() : ResourceID{};
        ++i;
    }
    return resourceIds;
}

template <typename T> Vector<ResourceHandle<T>> DeserializeResources(std::span<const ResourceID> ids, SerializationContext& context) {
    Vector<ResourceHandle<T>> result(ids.size());
    for (u32 i{}; auto& id : ids) {
        result[i] = context.resources.Get<T>(id);
        ++i;
    }
    return result;
}

void Serialize(nlohmann::json& json, const MeshComponent& comp) {
    json["model"] = SerializeResource(comp.modelInstance.GetModel());
    json["materials"] = SerializeResources(comp.modelInstance.GetMaterials().ToSpan());
    TO_JSON(instanced);
    TO_JSON(instanceTransformations);
}

void Deserialize(const nlohmann::json& json, SerializationContext& context, MeshComponent& comp) {
    JSON_DEFAULT_VAL();

    auto model{ DeserializeResource<Model>(json, "model", context.resources) };
    auto materialIds{ json.value<std::vector<ResourceID>>("materials", {}) };
    auto materials{ DeserializeResources<Material>(materialIds, context) };

    comp.modelInstance = ModelInstance{ model, std::move(materials) };
    FROM_JSON(instanced);
    FROM_JSON(instanceTransformations);
}

void Serialize(nlohmann::json& json, const AnimationControllerComponent& comp) {
    json["animation"] = SerializeResource(comp.animation);

    TO_JSON(isRunning);
    TO_JSON(speed);
    TO_JSON(resolutionS);
}

void Deserialize(const nlohmann::json& json, SerializationContext& context, AnimationControllerComponent& comp) {
    JSON_DEFAULT_VAL();

    comp.animation = DeserializeResource<Animation>(json, "animation", context.resources);
    FROM_JSON(isRunning);
    FROM_JSON(speed);
    FROM_JSON(resolutionS);
}

void Serialize(nlohmann::json& json, const SkyComponent& comp) {
    json["material"] = SerializeResource(comp.material);
}

void Deserialize(const nlohmann::json& json, SerializationContext& context, SkyComponent& comp) {
    JSON_DEFAULT_VAL();

    comp.material = DeserializeResource<Material>(json, "material", context.resources);
}

void Serialize(nlohmann::json& json, const LuaScriptComponent& comp) {
    json["script"] = SerializeResource(comp.script);
}

void Deserialize(const nlohmann::json& json, SerializationContext& context, LuaScriptComponent& comp) {
    JSON_DEFAULT_VAL();

    comp.script = DeserializeResource<LuaScript>(json, "script", context.resources);
}

COMPONENT(TagComponent, name, flags, layers, stencil)
COMPONENT(TransformationComponent, localTransformation, globalTransformation);
COMPONENT(ParentComponent, parent)
COMPONENT(RelationshipComponent, firstChild, lastChild, prevSibling, nextSibling)
COMPONENT(CameraComponent, isMain, projection, vFovDeg, zNear, zFar, width, height);
COMPONENT(LightComponent, type, intensity, color, range, spotAngleDeg, generatesShadows)

#define SERIALIZE_COMP(JSON, GO, TYPE)                                                                                                                         \
    if (go.Has<TYPE>()) {                                                                                                                                      \
        Serialize(JSON[#TYPE], go.Component<TYPE>());                                                                                                          \
    }

#define DESERIALIZE_COMP(JSON, CONTEXT, GO, TYPE)                                                                                                              \
    if (JSON.contains(#TYPE)) {                                                                                                                                \
        Deserialize(JSON[#TYPE], CONTEXT, go.GetOrCreateComponent<TYPE>());                                                                                    \
        go.Patch<TYPE>([](auto&) {});                                                                                                                          \
    }

nlohmann::json SerializeGO(GameObject go) {
    nlohmann::json json{};
    json["id"] = GameObjectHandle_t(go.Entity());
    json["name"] = go.Name();
    json["static"] = go.IsStatic();

    SERIALIZE_COMP(json, go, TagComponent);
    SERIALIZE_COMP(json, go, TransformationComponent);
    SERIALIZE_COMP(json, go, ParentComponent);
    SERIALIZE_COMP(json, go, RelationshipComponent);
    SERIALIZE_COMP(json, go, MeshComponent);
    SERIALIZE_COMP(json, go, LightComponent);
    SERIALIZE_COMP(json, go, CameraComponent);
    SERIALIZE_COMP(json, go, AnimationControllerComponent);
    SERIALIZE_COMP(json, go, SkyComponent);
    SERIALIZE_COMP(json, go, LuaScriptComponent);

    return json;
}

void DeserializeGO(nlohmann::json json, SerializationContext& context) {
    const auto name{ json.value<std::string>("name", "") };

    auto go{ context.world.CreateObject(name) };
    go.SetStatic(json.value("static", false));

    const GameObjectHandle localId{ json.value<GameObjectHandle_t>("id", {}) };
    context.idMap[localId] = go.Entity();

    context.deserialized.PushBack(go.Entity());

    DESERIALIZE_COMP(json, context, go, TagComponent);
    DESERIALIZE_COMP(json, context, go, TransformationComponent);
    DESERIALIZE_COMP(json, context, go, ParentComponent);
    DESERIALIZE_COMP(json, context, go, RelationshipComponent);
    DESERIALIZE_COMP(json, context, go, MeshComponent);
    DESERIALIZE_COMP(json, context, go, LightComponent);
    DESERIALIZE_COMP(json, context, go, CameraComponent);
    DESERIALIZE_COMP(json, context, go, AnimationControllerComponent);
    DESERIALIZE_COMP(json, context, go, SkyComponent);
    DESERIALIZE_COMP(json, context, go, LuaScriptComponent);
}

UGINE_FORCE_INLINE void FixId(GameObjectHandle& handle, const SerializationContext& context) {
    handle = handle == GameObjectNull ? GameObjectNull : context.idMap.at(handle);
}

void FixIds(SerializationContext& context) {
    // Fix serialized world ID's with real-world ID's.
    for (auto ent : context.deserialized) {
        auto go{ context.world.Get(ent) };

        // Sync global transformation.
        go.SetLocalTransformation(go.LocalTransformation());

        if (go.Has<TagComponent>()) {
            auto& tag{ go.Component<TagComponent>() };
            tag.id = StringID{ tag.name.c_str() };
        }

        if (go.Has<ParentComponent>()) {
            auto& parent{ go.Component<ParentComponent>() };
            FixId(parent.parent, context);
        }

        if (go.Has<RelationshipComponent>()) {
            auto& rel{ go.Component<RelationshipComponent>() };
            FixId(rel.firstChild, context);
            FixId(rel.lastChild, context);
            FixId(rel.nextSibling, context);
            FixId(rel.prevSibling, context);
        }
    }
}

void WorldSerializer::Serialize(World& world, const Path& path) {
    nlohmann::json j;

    world.Registry().each([&](auto entity) { j.push_back(SerializeGO(world.Get(entity))); });

    // TODO: ugine::OStream
    //Vector<u8> test;
    //OMemoryStream out{ test };
    std::ofstream out{ path.Data() };
    out << j;
}

void WorldSerializer::Deserialize(World& world, Engine& engine, const Path& path) {
    nlohmann::json j;

    try {
        Vector<u8> data;
        if (!engine.GetFileSystem().Read(path, data)) {
            return;
        }
        data.PushBack(0);
        j = nlohmann::json::parse(data.Data());
    } catch (const std::exception& ex) {
        UGINE_ERROR("Failed to deserialize world: {}", ex.what());
        return;
    }

    SerializationContext context{
        .world = world,
        .resources = engine.GetResources(),
    };

    for (auto& js : j) {
        DeserializeGO(js, context);
    }

    FixIds(context);
}

void WorldSerializer::Serialize(World& world, const GameObject& go, const Path& path) {
    nlohmann::json j;

    j.push_back(SerializeGO(go));

    Vector<GameObject> goStack{ go };
    while (!goStack.Empty()) {
        auto parent{ goStack.Back() };
        goStack.PopBack();

        auto child{ parent.FirstChild() };
        while (child) {
            if (child.HasChild()) {
                goStack.PushBack(child);
            }

            j.push_back(SerializeGO(child));
            child = child.NextSibling();
        }
    }

    std::ofstream out{ path.Data() };
    out << j;
}

} // namespace ugine