#pragma once

#include "../EditorEvents.h"
#include "../EditorTool.h"
#include "../window/SaveWindow.h"

#include <ugine/engine/world/World.h>

#include <ugine/Path.h>

#include <imgui.h>

namespace ugine::ed {

class WorldEditor : public EditorTool {
public:
    explicit WorldEditor(EditorContext& context);

    void Render() override;

    void PopulateGameObjects(World& world);

    void NewWorld();
    void OpenWorld(ResourceHandle<WorldDescriptor> handle, bool clear = true);
    void SaveWorld();

private:
    void OnOpenWorldResource(const OpenResourceEvent<WorldDescriptor>& event);
    void OnOpenTypelessResource(const OpenTypelessResourceEvent& event);
    void OnOpenWorld(const OpenWorldEvent& event);
    void OnSaveWorld(const SaveWorldEvent& event);
    void OnOpenProject(const OpenProjectEvent& event);

    void PopulateGameObject(GameObject& go, int& id);
    void PopulateIcons(const GameObject& go);

    void NewWorldTemplate(World* world) const;

    void RenderToolbar();
    void RenderSceneList();

    SaveWindow saveWindow_;
    std::optional<Path> worldPath_;
};

} // namespace ugine::ed