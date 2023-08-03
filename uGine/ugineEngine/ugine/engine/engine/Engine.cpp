#include "Engine.h"

#include <ugine/FileSystem.h>
#include <ugine/Log.h>
#include <ugine/Memory.h>
#include <ugine/Profile.h>
#include <ugine/String.h>
#include <ugine/Thread.h>

// Systems
#include <ugine/engine/core/CoreSystem.h>
#include <ugine/engine/engine/System.h>
#include <ugine/engine/gfx/GraphicsSystem.h>
#include <ugine/engine/gfx/ImGuiSystem.h>
#include <ugine/engine/input/InputSystem.h>
#include <ugine/engine/script/ScriptSystem.h>

#include <ugine/engine/core/ResourceManager.h>
#include <ugine/engine/system/Platform.h>
#include <ugine/engine/world/WorldManager.h>

#include <algorithm>
#include <chrono>

namespace ugine {

Engine::Engine(const EngineParams& params, IAllocator& allocator)
    : params_{ params }
    , mainThreadId_{ std::this_thread::get_id() }
    , allocator_{ allocator }
    , systems_{ allocator }
    , addedSystems_{ allocator }
    , removedSystems_{ allocator } {

    Init();
}

Engine::~Engine() {
    PROFILE_SHUTDOWN();

    worldManager_->DestroyWorlds();

    DestroySystems();

    resourceManager_ = nullptr;
    worldManager_ = nullptr;
    fileSystem_ = nullptr;

    DestroyScheduler();
    DestroyPlatform();
}

void Engine::Init() {
    PROFILE_THREAD("Main thread");

    config_.Load(StringView{ "ugine.ini" });

    time_.MaxDeltaMicros = 1000.0f * f32(std::chrono::milliseconds(config_.GetParam<u32>("core.maxDeltaMillis"_hs, 1000)).count());
    time_.startTime = Clock::now();
    time_.frameTime = Clock::now();

    InitLogger({});
    InitPlatform();
    InitScheduler();

    resourceManager_ = MakeUnique<ResourceManager>(allocator_, *this, allocator_);
    worldManager_ = MakeUnique<WorldManager>(allocator_, *this);

    fileSystem_ = FileSystem::Create(params_.rootPath, *scheduler_, THREAD_IO, allocator_);

    InitSystems(params_.systems);
}

void Engine::InitScheduler() {
    const auto tasks{ std::min<u32>(UGINE_MAX_THREADS, std::max<u32>(Thread::HardwareConcurency(), UGINE_MIN_THREADS)) };

    Vector<String> names{ tasks, allocator_ };
    names[0] = "Main thread";
    for (u32 i{ 1 }; i < tasks; ++i) {
        switch (i) {
        case THREAD_IO: names[i] = "IO thread"; break;
        default: names[i] = "Worker thread"; break;
        }
    }

    scheduler_ = MakeUnique<Scheduler>(allocator_, tasks, names.ToSpan(), allocator_);
    frameAllocators_.Resize(scheduler_->NumThreads());

    const auto size{ GetConfig().GetParam<u32>("engine.frameAllocationSize"_hs, 16 * 1024 * 1024) };
    for (u32 i{}; i < scheduler_->NumThreads(); ++i) {
        frameAllocators_[i].Init(size, allocator_);
    }
}

void Engine::DestroyScheduler() {
    scheduler_->ShutDown();
    scheduler_ = nullptr;
}

void Engine::BeginFrame() {
    time_.lastFrameTime = time_.frameTime;
    time_.frameTime = Clock::now();
    f32 microsDelta{ static_cast<f32>(std::chrono::duration_cast<std::chrono::microseconds>(time_.frameTime - time_.lastFrameTime).count()) };
    microsDelta = std::min<f32>(microsDelta, time_.MaxDeltaMicros);

    time_.timeDeltaMS = static_cast<f32>(microsDelta / 1000.0f);
    time_.timeDeltaS = static_cast<f32>(microsDelta / 1000000.0f);
}

void Engine::InitSystems(Systems systems) {
    if (systems & Systems::Core) {
        systems_.PushBack(MakeUnique<core::CoreSystem>(allocator_, *this));
    }

    if (systems & Systems::Input) {
        systems_.PushBack(MakeUnique<InputSystem>(allocator_, *this));
    }

    if (systems & Systems::Script) {
        systems_.PushBack(MakeUnique<ScriptSystem>(allocator_, *this));
    }

    if (systems & Systems::Graphics) {
        auto graphics{ MakeUnique<GraphicsSystem>(allocator_, *this) };
        auto pGraphics{ graphics.Get() };
        systems_.PushBack(std::move(graphics));

        if (params_.imgui) {
            auto gui{ MakeUnique<ImGuiSystem>(allocator_, *this) };
            pGraphics->AddLayer(gui.Get());
            systems_.PushBack(std::move(gui));
        }
    }

    for (auto& system : systems_) {
        system->Initialize();
    }
}

void Engine::DestroySystems() {
    fileSystem_->CancelAll();

    // TODO:
    GetResources().Clear<WorldDescriptor>();

    // Delete in reverse order.
    for (auto i{ systems_.Size() }; i > 0; --i) {
        systems_[i - 1].Reset();
    }

    systems_.Clear();
}

void Engine::AddRemoveSystems() {
    for (auto& newSystem : addedSystems_) {
        newSystem->Initialize();
        systems_.PushBack(std::move(newSystem));
    }

    if (!removedSystems_.Empty()) {
        systems_.EraseIf([&](auto& s) { return removedSystems_.IndexOf(s.Get()) >= 0; });
    }

    addedSystems_.Clear();
    removedSystems_.Clear();
}

void Engine::DestroyPlatform() {
    platform_ = nullptr;
}

void Engine::SetLogger(LoggerCallback callback) {
    InitLogger(callback);
}

void Engine::AddSystem(UniquePtr<System> system) {
    addedSystems_.PushBack(std::move(system));
}

void Engine::RemoveSystem(System* system) {
    const auto index{ systems_.FindIf([system](const auto& sys) { return sys.Get() == system; }) };

    if (index >= 0) {
        removedSystems_.PushBack(system);
    } else {
        // System not found, if it's newly added system just remove it.
        addedSystems_.EraseIf([system](const auto& sys) { return sys.Get() == system; });
    }
}

void Engine::InitPlatform() {
    platform_ = Platform::Create(params_, allocator_);
}

void Engine::ResetFrameAllocators() {
    for (auto& allocator : frameAllocators_) {
        allocator.Reset();
    }
}

int Engine::Run() {
    using namespace std::chrono;

    UGINE_INFO("Engine startup time: {}", FormatTime(Clock::now() - time_.startTime));

    try {
        while (!quitRequested_) {
            PROFILE_FRAME("MainThread");

            BeginFrame();

            while (auto event{ platform_->PollSystemEvent() }) {
                for (auto& system : systems_) {
                    system->HandleSystemEvent(*event);
                }

                if (event->type == SystemEvent::Type::Quit) {
                    quitRequested_ = true;
                    break;
                }
            }

            const auto frameStart{ Clock::now() };

            platform_->Update();

            auto& stats{ frameStats_[frameNumber_ % 2] };

            {
                PROFILE_EVENT_NC("Systems::EarlyUpdate", COLOR_PROFILE_ENGINE);

                const auto start{ Clock::now() };

                for (auto& system : systems_) {
                    system->EarlyUpdate();
                }

                stats.earlyUpdateMS = std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - start).count() / 1000.0f;
            }

            {
                PROFILE_EVENT_NC("Systems::Update", COLOR_PROFILE_ENGINE);

                const auto start{ Clock::now() };

                for (auto& system : systems_) {
                    system->Update();
                }

                stats.updateMS = std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - start).count() / 1000.0f;
            }

            {
                PROFILE_EVENT_NC("Systems::LateUpdate", COLOR_PROFILE_ENGINE);

                const auto start{ Clock::now() };

                for (auto& system : systems_) {
                    system->LateUpdate();
                }

                stats.lateUpdateMS = std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - start).count() / 1000.0f;
            }

            {
                PROFILE_EVENT_NC("Systems::Sync", COLOR_PROFILE_ENGINE);

                const auto start{ Clock::now() };

                for (auto& system : systems_) {
                    system->Sync();
                }

                stats.syncMS = std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - start).count() / 1000.0f;
            }

            AddRemoveSystems();

            {
                const auto start{ Clock::now() };

                fileSystem_->SyncPoint();

                stats.filesystemMS = std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - start).count() / 1000.0f;
            }

            worldManager_->SyncPoint();

            fpsCounter_.Fps(fps_);

            ResetFrameAllocators();

            // TODO: [multithread] Reset on sync point.
            frameAllocations_ = IAllocator::NumAllocs();
            IAllocator::ResetCounter();

            stats.frameTimeMS = std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - frameStart).count() / 1000.0f;

            ++frameNumber_;
        }
    } catch (const std::exception& ex) {
        UGINE_ERROR("Engine error: {}", ex.what());
    }

    return 0;
}

} // namespace ugine
