#include "World.h"
#include "WorldSerializer.h"

#include <ugine/engine/world/Component.h>

#include <ugine/Log.h>
#include <ugine/Profile.h>

#include <ugine/engine/math/Raycast.h>
#include <ugine/engine/utils/Debug.h>

namespace ugine {

World& World::FromRegistry(const GameObjectRegistry& reg) {
    auto storage{ reg.ctx().get<World*>() };

    UGINE_ASSERT(storage);

    return *storage;
}

World::World(private_constructor, IAllocator& allocator, u32 userFlags)
    : scenes_{ allocator }
    , userFlags_{ userFlags } {

    registry_.ctx().emplace<World*>(this);
}

World::~World() {
    // Destroy scenes before destroying registry.
    scenes_.Clear();

    registry_.each([&](const auto entity) { registry_.destroy(entity); });
}

WorldScene* World::GetScene(const StringID& name) {
    const auto index{ scenes_.FindIf([&](auto& scene) { return scene->Name() == name; }) };
    return index >= 0 ? scenes_[index].Get() : nullptr;
}

void World::AddScene(UniquePtr<WorldScene> scene) {
    scenes_.PushBack(std::move(scene));
}

//World World::Clone() const {
//    UGINE_ASSERT(false && "Not implemented");
//
//    return {};
//}

GameObject World::CreateObject(std::string_view name) {
    return GameObject::Create(registry_, name);
}

void World::DestroyObject(const GameObject& go) {
    GameObject::Destroy(go);
}

std::vector<GameObject> World::Find(std::string_view name) {
    const StringID id{ name.data() };

    std::vector<GameObject> result;
    for (auto&& [ent, tag] : registry_.view<TagComponent>().each()) {
        if (tag.id == id) {
            result.push_back(Get(ent));
        }
    }
    return result;
}

WorldHit World::RayCast(const Ray& ray) const {
    WorldHit hit{};

    for (const auto& scene : scenes_) {
        const auto sceneHit{ scene->RayCast(ray) };
        if (sceneHit.hit && (!hit.hit || hit.distance > sceneHit.distance)) {
            hit = sceneHit;
        }
    }

    return hit;
}

WorldHit World::RayCast(GameObject cameraGO, f32 screenX, f32 screenY) const {
    UGINE_ASSERT(cameraGO);
    UGINE_ASSERT(cameraGO.Has<CameraComponent>());

    // TODO:
    const auto& camera{ cameraGO.Component<CameraComponent>() };
    const auto ray{ RayFromCamera(
        ProjectionMatrix(camera), glm::inverse(cameraGO.GlobalTransformation().Matrix()), screenX, screenY, f32(camera.width), f32(camera.height)) };
    return RayCast(ray);
}

} // namespace ugine
