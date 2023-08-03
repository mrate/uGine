#pragma once

#include <gfxapi/Device.h>
#include <gfxapi/Swapchain.h>

#include <ugine/engine/engine/System.h>
#include <ugine/engine/world/WorldManager.h>

namespace ugine {

class Platform;
class GraphicsState;
class World;

class Layer {
public:
    virtual ~Layer() = default;

    virtual gfxapi::TextureHandle View() = 0;
    virtual bool Visible() const = 0;
};

class GraphicsSystem final : public System {
public:
    explicit GraphicsSystem(Engine& engine);
    ~GraphicsSystem();

    // System::*
    bool HandleSystemEvent(const SystemEvent& event) override;

    void Update() override;
    void Sync() override;

    void AddLayer(Layer* layer) { layers_.PushBack(layer); }

private:
    void OnWorldCreated(const WorldManager::WorldCreatedEvent& event);
    void OnWorldDestroyed(const WorldManager::WorldDestroyedEvent& event);

    void CreateSwapchain();

    UniquePtr<gfxapi::Device> device_;

    gfxapi::Swapchain* swapchain_{};
    Vector<gfxapi::FramebufferHandleUnique> swapchainFB_;
    gfxapi::RenderPassHandleUnique swapchainRenderpass_;
    gfxapi::GraphicsPipelineHandleUnique pipeline_;

    GraphicsState* state_{};

    Vector<Layer*> layers_;
};

} // namespace ugine