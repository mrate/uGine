#include "Resource.h"
#include "ResourceManager.h"

#include <ugine/Profile.h>
#include <ugine/StringUtils.h>

#include <ugine/engine/engine/Engine.h>

namespace ugine {

Resource::~Resource() {
    // Can't load virtual Unload() here.
    UGINE_ASSERT(state_ == ResourceState::Unloaded);
}

u64 Resource::IncRef() {
    return ++refCnt_;
}

u64 Resource::DecRef() {
    UGINE_ASSERT(refCnt_ > 0);

    if (--refCnt_ == 0) {
        Unload();
    }

    return refCnt_;
}

void Resource::LoadAsync(StringView file) {
    if (state_ != ResourceState::Unloaded) {
        return;
    }

    UGINE_DEBUG("Async loading {} '{}' from '{}'", Type().Name(), Id().ToString(), file.Data());
    UGINE_ASSERT(!ioRequest_);

    SetState(ResourceState::Loading);
    ioRequest_ = resourceManager_.GetEngine().GetFileSystem().ReadAsync(file, [this](Span<const u8> data, bool success) {
        ioRequest_ = {};
        if (success) {
            Load(data);
        } else {
            SetState(ResourceState::Failed);
        }
    });
}

void Resource::Load(Span<const u8> data) {
    UGINE_DEBUG("Loading resource {}: {}", Type().Name(), Id().ToString());
    UGINE_ASSERT(!ioRequest_);

    if (!HandleLoad(data)) {
        UGINE_WARN("{} load failed: {}", Type().Name(), Id().ToString());

        SetState(ResourceState::Failed);
        return;
    }

    if (loadingDependencies_ == 0) {
        UGINE_DEBUG("{} loaded: {}", Type().Name(), Id().ToString());
        SetState(ResourceState::Loaded);
    }
}

void Resource::Unload() {
    if (state_ == ResourceState::Unloaded) {
        return;
    }

    // TODO: Deffer unload.
    if (ioRequest_) {
        resourceManager_.GetEngine().GetFileSystem().Cancel(ioRequest_);
        ioRequest_ = {};
    }

    UGINE_DEBUG("Unloading resource {}: {}", Type().Name(), Id().ToString());

    if (HandleUnload()) {
        SetState(ResourceState::Unloaded);
    } else {
        SetState(ResourceState::Failed);
    }

    UGINE_ASSERT(dependencies_ == 0);
    UGINE_ASSERT(loadingDependencies_ == 0);
}

void Resource::AddDependency(Resource* resource) {
    UGINE_ASSERT(resource);

    ++dependencies_;

    UGINE_DEBUG("Adding dependency {} {} to {} {} (deps: {}, pending: {})", resource->Type().Name(), resource->Id().ToString(), Type().Name(),
        Id().ToString(), dependencies_, loadingDependencies_);

    resource->Connect<Resource::StateChangedEvent, &Resource::OnDependencyChanged>(this);
    if (resource->State() != ResourceState::Loaded) {
        ++loadingDependencies_;
    }
}

void Resource::RemoveDependency(Resource* resource) {
    UGINE_ASSERT(resource);

    UGINE_ASSERT(dependencies_ > 0);
    --dependencies_;

    UGINE_DEBUG("Removing dependency {} {} to {} {} (deps: {}, pending: {})", resource->Type().Name(), resource->Id().ToString(), Type().Name(),
        Id().ToString(), dependencies_, loadingDependencies_);

    resource->Disconnect<Resource::StateChangedEvent>(this);
    if (resource->State() != ResourceState::Loaded) {
        --loadingDependencies_;
    }
}

void Resource::OnDependencyChanged(const StateChangedEvent& event) {
    UGINE_DEBUG("Dependency changed for {} {}: {} => {}", Type().Name(), Id().ToString(), ResourceStateName[int(event.prevState)],
        ResourceStateName[int(event.newState)]);

    HandleDependencyChanged(event.id, event.newState);

    switch (event.newState) {
    case ResourceState::Unloaded:
        if (event.prevState == ResourceState::Loaded) {
            ++loadingDependencies_;
        }
        if (state_ == ResourceState::Loaded) {
            SetState(ResourceState::Unloaded);
        }
        break;
    case ResourceState::Loading:
        if (event.prevState == ResourceState::Loaded) {
            ++loadingDependencies_;
        }
        if (state_ == ResourceState::Loaded) {
            SetState(ResourceState::Loading);
        }
        break;
    case ResourceState::Loaded:
        if (event.prevState == ResourceState::Failed) {
            UGINE_ERROR("WTF?");
        }

        UGINE_ASSERT(loadingDependencies_ > 0);
        if (--loadingDependencies_ == 0) {
            UGINE_DEBUG("Dependencies loaded for {} {}", Type().Name(), Id().ToString());

            SetState(ResourceState::Loaded);
            HandleDependenciesReady();
        }
        break;
    case ResourceState::Failed:
        UGINE_ASSERT(loadingDependencies_ > 0);
        --loadingDependencies_;
        if (state_ != ResourceState::Failed) {
            SetState(ResourceState::Failed);
        }
        break;
    }
}

void Resource::SetState(ResourceState newState) {
    if (state_ != newState) {
        const auto prevState{ state_ };
        state_ = newState;

        if (prevState == ResourceState::Failed && newState == ResourceState::Loaded) {
            UGINE_ERROR("WTF?");
        }

        UGINE_DEBUG("Resource state changed '{}': {} => {}", id_.ToString(), ResourceStateName[int(prevState)], ResourceStateName[int(state_)]);

#ifdef UGINE_PROFILE
        {
            auto str{ std::format("'{}': {} => {}", id_.ToString(), ResourceStateName[int(prevState)], ResourceStateName[int(state_)]) };
            PROFILE_MESSAGE_DYN(str.c_str(), str.size());
        }
#endif

        Emit(StateChangedEvent{ Id(), this, state_, prevState });
    }
}

} // namespace ugine