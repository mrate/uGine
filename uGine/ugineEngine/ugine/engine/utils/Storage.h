#pragma once

#include <ugine/engine/utils/Debug.h>

#include <ugine/Log.h>
#include <ugine/Singleton.h>
#include <ugine/Ugine.h>

#include <ugine/engine/core/CoreProvider.h>

#include <entt/entt.hpp>

namespace ugine::utils {

template <typename T, typename _Type> class Storage : public Singleton<_Type> {
public:
    using ThisType = Storage<T, _Type>;
    using Id = GameObjectHandle;
    static const Id Invalid{ GameObjectNull };

    class Ref {
    public:
        Ref()
            : id_{ Invalid } {}

        explicit Ref(Id id)
            : id_{ id } {
            UGINE_ASSERT(core::CoreProvider::IsMainThread());

            if (id_ != Invalid) {
                ThisType::Instance().Lock(id_);
            }
        }

        template <typename X, typename _Y>
        Ref(const typename Storage<X, _Y>::Ref& other)
            : id_{ other.id_ } {
            UGINE_ASSERT(core::CoreProvider::IsMainThread());

            if (id_ != Invalid) {
                ThisType::Instance().Lock(id_);
            }
        }

        template <typename X, typename _Y> Ref(typename Storage<X, _Y>::Ref&& other) {
            UGINE_ASSERT(core::CoreProvider::IsMainThread());

            id_ = other.id_;
            other.id_ = Invalid;
        }

        Ref(const Ref& other)
            : id_{ other.id_ } {
            UGINE_ASSERT(core::CoreProvider::IsMainThread());

            if (id_ != Invalid) {
                ThisType::Instance().Lock(id_);
            }
        }

        Ref& operator=(const Ref& other) {
            UGINE_ASSERT(core::CoreProvider::IsMainThread());

            if (id_ != Invalid) {
                ThisType::Instance().Unlock(id_);
            }
            id_ = other.id_;
            if (id_ != Invalid) {
                ThisType::Instance().Lock(id_);
            }

            return *this;
        }

        Ref(Ref&& other) {
            id_ = other.id_;
            other.id_ = Invalid;
        }

        Ref& operator=(Ref&& other) {
            if (id_ != Invalid) {
                ThisType::Instance().Unlock(id_);
            }
            id_ = other.id_;
            other.id_ = Invalid;

            return *this;
        }

        ~Ref() {
            if (id_ != Invalid) {
                ThisType::Instance().Unlock(id_);
            }
        }

        T& Value() {
            UGINE_ASSERT(id_ != Invalid);

            return ThisType::Instance().Get(id_);
        }

        T& operator*() { return Value(); }

        T* operator->() { return Ptr(); }

        const T& Value() const {
            UGINE_ASSERT(id_ != Invalid);

            return ThisType::Instance().Get(id_);
        }

        T* Ptr() const {
            UGINE_ASSERT(id_ != Invalid);

            return &ThisType::Instance().Get(id_);
        }

        T* operator&() const { return Ptr(); }

        const T& operator*() const { return Value(); }

        const T* operator->() const { return Ptr(); }

        operator bool() const { return id_ != Invalid; }

        bool operator==(const Ref& other) const { return id_ == other.id_; }

        bool operator!=(const Ref& other) const { return !(*this == other); }

        Id id() const { return id_; }

#ifdef _DEBUG
        void _DebugAttach(const Debug& debug) {
            UGINE_ASSERT(core::CoreProvider::IsMainThread());

            ThisType::Instance().Attach<Debug>(*this, debug);
        }

        void _DebugDetach(const Debug& debug) {
            UGINE_ASSERT(core::CoreProvider::IsMainThread());

            ThisType::Instance().Detach<Debug>(*this);
        }

        const Debug& _DebugInfo() const {
            UGINE_ASSERT(core::CoreProvider::IsMainThread());

            return ThisType::Instance().Get<Debug>(*this);
        }
#endif

    private:
        Id id_{};
    };

    struct DebugTrace {
        const char* fileName{};
        int line{};
    };

    virtual ~Storage() {}

    static void Init(const std::string_view& name) {
        UGINE_ASSERT(core::CoreProvider::IsMainThread());

        ThisType::Instance().name_ = name;
#ifdef _DEBUG
        ThisType::Instance().storage_.on_construct<Debug>().connect<&OnDebugAttach>();
        ThisType::Instance().storage_.on_destroy<Debug>().connect<&OnDebugDetach>();
#endif
    }

    static void Destroy() {
        UGINE_ASSERT(core::CoreProvider::IsMainThread());

#ifdef _DEBUG
        auto prevSize{ Size() };
#endif
        ThisType::Instance().storage_.each([](auto entt) { Remove(entt); });
        ThisType::Instance().storage_.orphans([](auto entt) { Remove(entt); });

#ifdef _DEBUG
        auto newSize{ Size() };
        if (newSize) {
            UGINE_DEBUG("{} of {} not deleted", newSize, Name());
        }
        TraceObjects();
#endif
        UGINE_ASSERT(Size() == 0);
    }

    static Ref Add(T&& item, const char* file = nullptr, int line = 0) {
        UGINE_ASSERT(core::CoreProvider::IsMainThread());

        auto ent{ ThisType::Instance().storage_.create() };
        ThisType::Instance().storage_.emplace<T>(ent, std::move(item));
        ThisType::Instance().storage_.emplace<u32>(ent, 0);

        if (file) {
            ThisType::Instance().storage_.emplace<DebugTrace>(ent, file, line);
        }

        return Ref{ ent };
    }

    template <typename... Args> static Ref Add(const char* file, int line, Args&&... args) { return Add(T(std::forward<Args>(args)...), file, line); }

    static void TraceObjects() {
        for (auto [ent, debug] : ThisType::Instance().storage_.view<DebugTrace>().each()) {
            UGINE_DEBUG("Zombie {}: {}:{}", Name(), debug.fileName, debug.line);
        }
    }

    static const DebugTrace* TraceObject(Id id) {
        auto& storage{ ThisType::Instance().storage_ };
        return storage.has<DebugTrace>(id) ? &storage.get<DebugTrace>() : nullptr;
    }

    static void Remove(Id id) {
        UGINE_ASSERT(core::CoreProvider::IsMainThread());

        ThisType::Instance().PreDestroy(id);

        UGINE_ASSERT(ThisType::Instance().storage_.has<T>(id));
        UGINE_ASSERT(ThisType::Instance().storage_.has<u32>(id));

        ThisType::Instance().storage_.destroy(id);

        ThisType::Instance().PostDestroy(id);
    }

    static void Replace(Id id, const T& newValue) {
        UGINE_ASSERT(core::CoreProvider::IsMainThread());

        ThisType::Instance().PreReplace(id);
        ThisType::Instance().storage_.replace<T>(id, newValue);
        ThisType::Instance().PostReplace(id);
    }

    static void Replace(Id id, T&& newValue) {
        UGINE_ASSERT(core::CoreProvider::IsMainThread());

        ThisType::Instance().PreReplace(id);
        ThisType::Instance().storage_.patch<T>(id, [&](auto& v) { v = std::move(newValue); });
        ThisType::Instance().PostReplace(id);
    }

    static void Lock(Id id) {
        UGINE_ASSERT(core::CoreProvider::IsMainThread());

        UGINE_ASSERT(ThisType::Instance().storage_.has<T>(id));
        UGINE_ASSERT(ThisType::Instance().storage_.has<u32>(id));

        ++ThisType::Instance().storage_.get<u32>(id);
    }

    static void Unlock(Id id) {
        UGINE_ASSERT(core::CoreProvider::IsMainThread());

        //UGINE_ASSERT(ThisType::Instance().storage_.has<T>(id));
        //UGINE_ASSERT(ThisType::Instance().storage_.has<u32>(id));
        UGINE_ASSERT(ThisType::Instance().storage_.get<u32>(id) > 0);

        auto debug{ ThisType::Instance().storage_.get<u32>(id) };

        if (--ThisType::Instance().storage_.get<u32>(id) == 0) {
            Remove(id);
        }
    }

    static T& Get(Id id) {
        //UGINE_ASSERT(ThisType::Instance().storage_.has<T>(id));
        //UGINE_ASSERT(ThisType::Instance().storage_.has<u32>(id));

        return ThisType::Instance().storage_.get<T>(id);
    }

    static size_t Size() { return ThisType::Instance().storage_.alive(); }

    static u32 Count(const Ref& ref) { return Count(ref.id()); }

    static u32 Count(Id id) { return ThisType::Instance().storage_.get<u32>(id); }

    template <typename _A, typename... _Args> static _A& Attach(Id id, _Args&&... args) {
        UGINE_ASSERT(core::CoreProvider::IsMainThread());

        return ThisType::Instance().storage_.emplace<_A>(id, std::forward<_Args>(args)...);
    }

    template <typename _A> static void Detach(Id id) {
        UGINE_ASSERT(core::CoreProvider::IsMainThread());

        ThisType::Instance().storage_.destroy<_A>(id);
    }

    template <typename _A> static _A& Get(Id id) { return ThisType::Instance().storage_.get<_A>(id); }

    template <typename _A> static bool Has(Id id) { return ThisType::Instance().storage_.has<_A>(id); }

    template <typename _A, typename... _Args> static _A& Attach(const Ref& item, _Args&&... args) {
        return Attach<_A>(item.id(), std::forward<_Args>(args)...);
    }

    template <typename _A> static void Detach(const Ref& item) { Detach<_A>(item.id); }

    template <typename _A> static _A& Get(const Ref& item) { return Get<_A>(item.id()); }

    template <typename _A> static bool Has(const Ref& item) { return Has<_A>(item.id()); }

    template <typename T> static void Each(T&& t) {
        UGINE_ASSERT(core::CoreProvider::IsMainThread());

        ThisType::Instance().storage_.each([&](Id id) { t(Ref{ id }); });
    }

    static const std::string& Name() { return ThisType::Instance().name_; }

protected:
    virtual void PreDestroy(Id id) {}
    virtual void PostDestroy(Id id) {}
    virtual void PreReplace(Id id) {}
    virtual void PostReplace(Id id) {}

    GameObjectRegistry storage_;
    std::string name_;
};

#define STORAGE_ADD(Storage, ...) Storage::Add(__FILE__, __LINE__, __VA_ARGS__)

} // namespace ugine::utils
