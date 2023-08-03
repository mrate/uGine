#pragma once

#include <ugine/engine/core/ResourceManager.h>
#include <ugine/engine/world/GameObject.h>

#include <ugine/Utils.h>

#include <string_view>

namespace ugine {

// Simple resource that holds only the path to world file.
class WorldDescriptor final : public Resource {
public:
    inline static const ResourceType TYPE{ "WorldDescriptor" };

    explicit WorldDescriptor(ResourceManager& resourceManager, const ResourceID& id)
        : Resource{ resourceManager, TYPE, id } {}
};

class WorldManager;
struct Ray;

template <typename T> void SafeRemoveComponent(GameObjectRegistry& reg, GameObjectHandle ent) {
    if (reg.any_of<T>(ent)) {
        reg.erase<T>(ent);
    }
}

struct WorldHit {
    bool hit{};
    float distance{};
    GameObjectHandle object{};
    glm::vec3 point{};
    glm::vec2 uv{};
    glm::vec3 triangle[3];
    // TODO: Component.
    u32 meshIndex{};
};

class WorldScene {
public:
    WorldScene(Engine& engine, World& world)
        : engine_{ engine }
        , world_{ world } {}
    virtual ~WorldScene() = default;

    virtual WorldHit RayCast(const Ray& ray) const { return WorldHit{}; }

    virtual void Update() = 0;
    virtual StringID Name() const = 0;

    World& GetWorld() { return world_; }

protected:
    Engine& engine_;
    World& world_;
};

class World final {
private:
    // TODO: For now
    friend WorldManager;
    struct private_constructor {};

public:
    UGINE_MOVABLE_ONLY(World);

    static World& FromRegistry(const GameObjectRegistry& reg);

    World(private_constructor, IAllocator& allocator = IAllocator::Default(), u32 userFlags = 0);
    ~World();

    void SetName(StringView name) { name_ = name; }

    void SetUserFlags(u32 flags) { userFlags_ = flags; }
    u32 GetUserFlags() const { return userFlags_; }

    // Scene management.
    WorldScene* GetScene(const StringID& name);
    template <typename T> T* GetScene() { return static_cast<T*>(GetScene(T::NAME)); }
    void AddScene(UniquePtr<WorldScene> scene);

    bool IsSimulating() const { return flags_.simulating; }
    void SetSimulating(bool simulating) { flags_.simulating = simulating ? 1 : 0; }

    bool IsRendering() const { return flags_.rendering; }
    void SetRendering(bool rendering) { flags_.rendering = rendering ? 1 : 0; }

    // Game object management.
    GameObject CreateObject(std::string_view name);
    void DestroyObject(const GameObject& go);

    u32 Size() const { return static_cast<u32>(registry_.alive()); }
    void Clear() { registry_.clear(); }

    template <class Func> void Each(Func f) {
        registry_.each([&](auto ent) { f(GameObject{ registry_, ent }); });
    }

    GameObject Get(GameObjectHandle ent) { return GameObject{ registry_, ent }; }
    std::vector<GameObject> Find(std::string_view name);

    template <typename T> GameObject GameObjectFromComponent(T&& t) { return GameObject{ registry_, entt::to_entity(registry_, t) }; }
    template <typename T> size_t Count() { return registry_.view<T>().size(); }

    GameObjectRegistry& Registry() { return registry_; }
    const GameObjectRegistry& Registry() const { return registry_; }

    // Other.
    WorldHit RayCast(const Ray& ray) const;
    WorldHit RayCast(GameObject cameraGO, f32 screenX, f32 screenY) const;

private:
    static UniquePtr<World> Create(IAllocator& allocator, u32 userFlags = 0) {
        return MakeUnique<World>(allocator, private_constructor{}, allocator, userFlags);
    }

    GameObjectRegistry registry_;

    struct {
        u8 simulating : 1;
        u8 rendering  : 1;
    } flags_{};

    Vector<UniquePtr<WorldScene>> scenes_;

    String name_;
    u32 userFlags_{};
};

} // namespace ugine
