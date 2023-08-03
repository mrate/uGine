#pragma once

namespace ugine {

class Engine;
class GameObject;
class World;
class Path;

class WorldSerializer {
public:
    WorldSerializer() = default;

    void Serialize(World& world, const Path& path);
    void Serialize(World& world, const GameObject& go, const Path& path);

    void Deserialize(World& world, Engine& engine, const Path& path);
};

} // namespace ugine