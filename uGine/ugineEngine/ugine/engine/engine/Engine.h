#pragma once

#include "Config.h"
#include "Params.h"

#include <ugine/engine/system/Event.h>
#include <ugine/engine/system/Platform.h>

#include <ugine/FileSystem.h>
#include <ugine/FpsCounter.h>
#include <ugine/Log.h>
#include <ugine/Memory.h>
#include <ugine/Scheduler.h>
#include <ugine/Vector.h>

#include <entt/entt.hpp>

#include <chrono>

namespace ugine {

class Allocator;
class ResourceManager;
class Platform;
class WorldManager;
class System;

class Engine {
public:
    struct FrameStats {
        float earlyUpdateMS{};
        float updateMS{};
        float lateUpdateMS{};
        float syncMS{};
        float filesystemMS{};
        float frameTimeMS{};
    };

    Engine(const EngineParams& params, IAllocator& allocator = IAllocator::Default());
    ~Engine();

    int Run();
    void Quit() { quitRequested_ = true; }
    bool IsQuitting() const { return quitRequested_; }

    // Properties.
    u32 GetFrameAllocations() const { return frameAllocations_; }

    const EngineParams& GetParams() const { return params_; }
    const Configuration& GetConfig() const { return config_; }

    u64 FrameNumber() const { return frameNumber_; }
    float Fps() const { return fps_; }

    // Subsystems.
    IAllocator& GetAllocator() const { return allocator_; }
    Platform& GetPlatform() { return *platform_; }
    WorldManager& GetWorldManager() { return *worldManager_; }
    ResourceManager& GetResources() { return *resourceManager_; }
    Scheduler& GetScheduler() { return *scheduler_; }
    FileSystem& GetFileSystem() { return *fileSystem_; }

    void SetLogger(LoggerCallback callback);

    // Time of frame start.
    const FrameStats& GetFrameStats() const { return frameStats_[(frameNumber_ + 1) % 2]; }

    f64 FrameSeconds() const { return f32(std::chrono::duration_cast<std::chrono::microseconds>(time_.frameTime - time_.startTime).count()) / 1e6f; }
    f64 FrameMillis() const { return f32(std::chrono::duration_cast<std::chrono::microseconds>(time_.frameTime - time_.startTime).count()) / 1e3f; }

    f64 FrameSecondsDelta() const { return time_.timeDeltaS; }
    f64 FrameMillisDelta() const { return time_.timeDeltaMS; }

    // Time since start.
    f64 Seconds() const { return f32(std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - time_.startTime).count()) / 1e6f; }
    f64 Millis() const { return f32(std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - time_.startTime).count()) / 1e3f; }

    bool IsMainThread() const { return std::this_thread::get_id() == mainThreadId_; }
    // TODO: RunOnMainThread(...)

    // Systems.
    void AddSystem(UniquePtr<System> system);
    void RemoveSystem(System* system);

    // Memory
    IAllocator& FrameAllocator(u32 threadNum = Scheduler::ThreadNum()) { return frameAllocators_[threadNum]; }

    // States.

    template <typename T, typename... Args> T& AttachState(Args&&... args) { return states_.ctx().emplace<T>(std::forward<Args>(args)...); }
    template <typename T> T* GetState() { return states_.ctx().contains<T>() ? &states_.ctx().get<T>() : nullptr; }
    template <typename T> const T* GetState() const { return states_.ctx().contains<T>() ? &states_.ctx().get<T>() : nullptr; }
    template <typename T> void DeleteState() { states_.ctx().erase<T>(); }

private:
    using Clock = std::chrono::high_resolution_clock;
    static constexpr u32 THREAD_IO{ 1 };

    struct Time {
        f32 MaxDeltaMicros{};

        Clock::time_point startTime{};
        Clock::time_point frameTime{};
        Clock::time_point lastFrameTime{};
        f32 timeDeltaMS{};
        f32 timeDeltaS{};
    };

    void Init();
    void BeginFrame();

    void InitScheduler();
    void DestroyScheduler();

    void InitPlatform();
    void DestroyPlatform();

    void InitSystems(Systems systems);
    void DestroySystems();
    void AddRemoveSystems();

    void ResetFrameAllocators();

    const EngineParams params_{};
    const std::thread::id mainThreadId_{};
    AllocatorRef allocator_;

    Configuration config_;

    bool quitRequested_{};

    UniquePtr<Platform> platform_;

    Vector<UniquePtr<System>> systems_;
    Vector<UniquePtr<System>> addedSystems_;
    Vector<System*> removedSystems_;

    Time time_;

    UniquePtr<ResourceManager> resourceManager_;
    UniquePtr<WorldManager> worldManager_;

    UniquePtr<Scheduler> scheduler_;
    UniquePtr<FileSystem> fileSystem_;

    u64 frameNumber_{};
    FpsCounter fpsCounter_;
    float fps_{};
    u32 frameAllocations_{};

    // Attached states.
    entt::registry states_;

    Vector<LinearAllocator> frameAllocators_;

    FrameStats frameStats_[2];
};

} // namespace ugine
