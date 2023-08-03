#include <fstream>
#include <iostream>

#include <ugine/FileSystem.h>

#include <ugine/engine/core/Json.h>
#include <ugine/engine/core/ResourceManager.h>
#include <ugine/engine/engine/Engine.h>
#include <ugine/engine/engine/System.h>
#include <ugine/engine/gfx/Component.h>
#include <ugine/engine/gfx/GraphicsScene.h>
#include <ugine/engine/gfx/GraphicsState.h>
#include <ugine/engine/gfx/GraphicsSystem.h>
#include <ugine/engine/world/WorldManager.h>
#include <ugine/engine/world/WorldSerializer.h>

using namespace ugine;

class InitSystem : public System {
public:
    explicit InitSystem(Engine& engine)
        : System{ engine } {
        ScanDirectory(FileSystem::CurrentPath());
    }

    ~InitSystem() {}

    void Update() {
        auto newWorld{ GetEngine().GetWorldManager().CreateWorld() };

        WorldSerializer ser{};

        // TODO:
        ser.Deserialize(*newWorld, GetEngine(), Path{ "script//ScriptScene.uworld" });
        newWorld->SetRendering(true);
        newWorld->SetSimulating(true);

        for (auto&& [ent, camera] : newWorld->Registry().view<CameraComponent>().each()) {
            camera.width = 1600;
            camera.height = 1200;
        }

        GetEngine().GetWorldManager().SetDefaultWorld(newWorld);

        GetEngine().RemoveSystem(this);
    }

    void ScanDirectory(const Path& dir) {
        // TODO: Filesystem.
        for (const auto& it : DirectoryIterator{ dir }) {
            if (it.IsDirectory()) {
                ScanDirectory(it.Path());
            } else if (it.IsRegularFile() && it.Path().Extension() == ".meta") {
                AddResourceFile(it.Path());
            }
        }
    }

    void AddResourceFile(const Path& metaFile) {
        // TODO:

        try {
            nlohmann::json json{};
            {
                std::ifstream in{ metaFile.Data() };
                in >> json;
            }

            const auto id{ json["id"].get<ResourceID>() };
            const auto type{ ResourceType{ json["type"].get<std::string>() } };
            auto resourcePath{ metaFile };
            resourcePath.ReplaceExtension("");

            if (FileSystem::Exists(resourcePath)) {
                GetEngine().GetResources().RegisterResource(id, Path::RelativeDirectory(resourcePath, FileSystem::CurrentPath()), type);
            }
        } catch (const std::exception& ex) {
            // TODO:
            UGINE_ERROR("Failed to add resource file '{}': {}", metaFile.String(), ex.what());
        }
    }
};

class UpdateSystem : public System {
public:
    explicit UpdateSystem(Engine& engine)
        : System{ engine } {}

    void Update() {
        auto world{ GetEngine().GetWorldManager().GetDefaultWorld() };
        UGINE_ASSERT(world);

        // TODO:
        for (auto&& [ent, camera] : world->Registry().view<CameraComponent>().each()) {
            //if (camera.isMain) {
            GetEngine().GetState<GraphicsState>()->mainOutputOverride = world->GetScene<GraphicsScene>()->GetCameraRtv(world->Get(ent));
            break;
            //}
        }
    }
};

int main(int argc, char* argv[]) {
    try {
        EngineParams params = {
            .appName = "uGine Player",
            .appVersion = { 1, 0, 0 },
            .systems = Systems::All,
            .fullscreen = false,
            .width = 1600,
            .height = 1200,
#ifdef _DEBUG
            .debugGraphics = true,
#else
            .debugGraphics = false,
#endif
            .imgui = false,
        };

        ugine::Engine engine{ params };

        engine.AddSystem(MakeUnique<InitSystem>(engine.GetAllocator(), engine));
        engine.AddSystem(MakeUnique<UpdateSystem>(engine.GetAllocator(), engine));

        engine.Run();

    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
