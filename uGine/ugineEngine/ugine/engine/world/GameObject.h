#pragma once

#include <ugine/engine/world/Component.h>
#include <ugine/engine/world/Transformation.h>

#include <ugine/Json.h>
#include <ugine/String.h>
#include <ugine/Ugine.h>
#include <ugine/engine/math/Math.h>

#include <entt/entt.hpp>

#include <glm/glm.hpp>

#include <string>
#include <string_view>

namespace ugine {

class World;

using GameObjectRegistry = entt::registry;
using GameObjectHandle = entt::entity;
using GameObjectHandle_t = std::underlying_type_t<GameObjectHandle>;

inline static constexpr auto GameObjectNull{ entt::null };

struct TagComponent {
    static constexpr u32 MAX_NAME_LENGTH{ 32 };

    static TagComponent Init(std::string_view name) {
        TagComponent tag{};
        tag.name = name;
        tag.id = StringID{ name.data() };
        return tag;
    }

    enum Flags {
        Editor = UGINE_BIT(0),
        Disabled = UGINE_BIT(1),
    };

    std::string name; // TODO:
    StringID id;
    u32 flags{};
    u32 layers{ u32(-1) };
    u8 stencil{}; // TODO: Here for now.
};

struct StaticFlagComponent {};

struct ParentComponent {
    GameObjectHandle parent{ GameObjectNull };
};

struct RelationshipComponent {
    u32 children{};
    GameObjectHandle firstChild{ GameObjectNull };
    GameObjectHandle lastChild{ GameObjectNull };
    GameObjectHandle prevSibling{ GameObjectNull };
    GameObjectHandle nextSibling{ GameObjectNull };
};

template <typename T> struct NonModifiableComponent : public std::true_type {};

template <> struct NonModifiableComponent<TagComponent> : public std::false_type {};
template <> struct NonModifiableComponent<RelationshipComponent> : public std::false_type {};
template <> struct NonModifiableComponent<ParentComponent> : public std::false_type {};
template <> struct NonModifiableComponent<TransformationComponent> : public std::false_type {};
template <> struct NonModifiableComponent<StaticFlagComponent> : public std::false_type {};

class GameObject {
public:
    static GameObject Create(GameObjectRegistry& registry, std::string_view name);
    static void Destroy(const GameObject& go);

    GameObject() = default;

    GameObject(GameObjectRegistry& registry, GameObjectHandle entity)
        : handle_{ registry, entity } {}

    UGINE_FORCE_INLINE GameObject Parent() const {
        auto res{ handle_.try_get<ParentComponent>() };
        return res ? GameObject{ *handle_.registry(), handle_.get<ParentComponent>().parent } : GameObject{};
    }

    UGINE_FORCE_INLINE World* World() const { return IsValid() ? handle_.registry()->ctx().get<ugine::World*>() : nullptr; }

    UGINE_FORCE_INLINE bool IsValid() const { return handle_.entity() != GameObjectNull; }

    UGINE_FORCE_INLINE void SetStatic(bool isStatic) {
        if (isStatic) {
            handle_.emplace_or_replace<StaticFlagComponent>();
        } else {
            handle_.remove<StaticFlagComponent>();
        }
    }

    UGINE_FORCE_INLINE bool IsStatic() const { return handle_.any_of<StaticFlagComponent>(); }

    UGINE_FORCE_INLINE const std::string& Name() const {
        UGINE_ASSERT(handle_.try_get<TagComponent>() != nullptr);
        return handle_.get<TagComponent>().name;
    }

    UGINE_FORCE_INLINE void SetName(std::string_view name) {
        UGINE_ASSERT(handle_.try_get<TagComponent>() != nullptr);
        handle_.patch<TagComponent>([&](auto& t) {
            t.name = name;
            t.id = StringID{ t.name.data() };
        });
    }

    UGINE_FORCE_INLINE bool IsEnabled() const {
        UGINE_ASSERT(handle_.try_get<TagComponent>() != nullptr);
        return !(handle_.get<TagComponent>().flags & TagComponent::Flags::Disabled);
    }

    UGINE_FORCE_INLINE void SetEnabled(bool enabled) {
        UGINE_ASSERT(handle_.try_get<TagComponent>() != nullptr);
        handle_.patch<TagComponent>(
            [enabled](TagComponent& t) { t.flags = enabled ? (t.flags & ~TagComponent::Flags::Disabled) : (t.flags | TagComponent::Flags::Disabled); });
    }

    UGINE_FORCE_INLINE u8 GetStencil() const {
        UGINE_ASSERT(handle_.try_get<TagComponent>() != nullptr);
        return handle_.get<TagComponent>().stencil;
    }

    UGINE_FORCE_INLINE void SetStencil(u8 stencil) {
        UGINE_ASSERT(handle_.try_get<TagComponent>() != nullptr);
        handle_.patch<TagComponent>([stencil](TagComponent& t) { t.stencil = stencil; });
    }

    UGINE_FORCE_INLINE u32 Flags() const {
        UGINE_ASSERT(handle_.try_get<TagComponent>() != nullptr);
        return handle_.get<TagComponent>().flags;
    }

    UGINE_FORCE_INLINE void SetFlags(u32 flags) {
        UGINE_ASSERT(handle_.try_get<TagComponent>() != nullptr);
        handle_.patch<TagComponent>([flags](TagComponent& t) { t.flags = flags; });
    }

    UGINE_FORCE_INLINE bool HasChild() const { return handle_.get<RelationshipComponent>().children > 0; }
    UGINE_FORCE_INLINE u32 ChildrenCount() const { return handle_.get<RelationshipComponent>().children; }

    void AddChild(GameObjectHandle child);
    void RemoveChild(GameObjectHandle child);

    UGINE_FORCE_INLINE GameObject PrevSibling() const {
        const auto& rel{ handle_.get<RelationshipComponent>() };
        return rel.prevSibling == GameObjectNull ? GameObject{} : GameObject(*handle_.registry(), rel.prevSibling);
    }

    UGINE_FORCE_INLINE GameObject NextSibling() const {
        const auto& rel{ handle_.get<RelationshipComponent>() };
        return rel.nextSibling == GameObjectNull ? GameObject{} : GameObject(*handle_.registry(), rel.nextSibling);
    }

    UGINE_FORCE_INLINE GameObject FirstChild() const {
        const auto& rel{ handle_.get<RelationshipComponent>() };
        return rel.firstChild == GameObjectNull ? GameObject{} : GameObject(*handle_.registry(), rel.firstChild);
    }

    template <typename T> UGINE_FORCE_INLINE decltype(auto) Component() {
        UGINE_ASSERT(Has<T>());
        return handle_.get<T>();
    }

    template <typename T> UGINE_FORCE_INLINE decltype(auto) Component() const { return handle_.get<T>(); }

    template <typename T> UGINE_FORCE_INLINE decltype(auto) TryGetComponent() {
        static_assert(NonModifiableComponent<T>::value, "Cannot return pointer to this component type");

        return handle_.try_get<T>();
    }

    template <typename T> UGINE_FORCE_INLINE const T* TryGetComponent() const { return handle_.try_get<T>(); }

    template <typename T> UGINE_FORCE_INLINE bool Has() const { return handle_.any_of<T>(); }

    template <typename T, typename... Args> UGINE_FORCE_INLINE decltype(auto) CreateComponent(Args&&... args) {
        return handle_.emplace<T>(std::forward<Args>(args)...);
    }

    template <typename T> UGINE_FORCE_INLINE void RemoveComponent() {
        static_assert(NonModifiableComponent<T>::value, "Cannot remove this component type");

        handle_.remove<T>();
    }

    template <typename T, typename... Args> UGINE_FORCE_INLINE decltype(auto) GetOrCreateComponent(Args&&... args) {
        return handle_.get_or_emplace<T>(std::forward<Args>(args)...);
    }

    template <typename T, typename... Args> UGINE_FORCE_INLINE decltype(auto) Replace(Args&&... args) {
        static_assert(NonModifiableComponent<T>::value, "Cannot replace this component type");

        return handle_.replace<T>(std::forward<Args>(args)...);
    }

    template <typename T, typename... Args> UGINE_FORCE_INLINE decltype(auto) Patch(Args&&... args) {
        // TODO: Turned off for deserialization...
        //static_assert(NonModifiableComponent<T>::value, "Cannot patch this component type");

        return handle_.patch<T>(std::forward<Args>(args)...);
    }

    void MoveTo(const glm::vec3& to) {
        auto trans{ GlobalTransformation() };
        trans.position = to;
        SetGlobalTransformation(trans);
    }

    UGINE_FORCE_INLINE void MoveTo(f32 x, f32 y, f32 z) { MoveTo({ x, y, z }); }

    void Scale(const glm::vec3& scale) {
        auto trans{ GlobalTransformation() };
        trans.scale = scale;
        SetGlobalTransformation(trans);
    }

    UGINE_FORCE_INLINE void Scale(f32 x, f32 y, f32 z) { Scale({ x, y, z }); }

    void Scale(f32 scale) { Scale({ scale, scale, scale }); }

    void Rotate(const glm::fquat& quat) {
        auto trans{ GlobalTransformation() };
        trans.rotation = quat;
        SetGlobalTransformation(trans);
    }

    UGINE_FORCE_INLINE void Rotate(const glm::vec3& euler) { Rotate(glm::fquat(euler)); }
    UGINE_FORCE_INLINE void Rotate(f32 x, f32 y, f32 z) { Rotate(glm::fquat(glm::vec3{ x, y, z })); }

    void LookAt(const glm::vec3& to) {
        auto trans{ GlobalTransformation() };
        trans.rotation = ugine::LookAt(to - trans.position, math::UP);
        SetGlobalTransformation(trans);
    }

    UGINE_FORCE_INLINE void LookAt(const GameObject& other) { LookAt(other.GlobalTransformation().position); }

    UGINE_FORCE_INLINE operator bool() const { return IsValid(); }
    UGINE_FORCE_INLINE operator u32() const { return static_cast<u32>(handle_.entity()); }
    UGINE_FORCE_INLINE operator GameObjectHandle() const { return handle_.entity(); }

    UGINE_FORCE_INLINE GameObjectHandle Entity() const { return handle_.entity(); }

    UGINE_FORCE_INLINE const Transformation& GlobalTransformation() const { return Component<TransformationComponent>().globalTransformation; }
    UGINE_FORCE_INLINE const Transformation& LocalTransformation() const { return Component<TransformationComponent>().localTransformation; }

    void SetGlobalTransformation(const Transformation& globalTransform) {
        auto& comp{ Component<TransformationComponent>() };

        comp.globalTransformation = globalTransform;
        if (Parent()) {
            comp.localTransformation = Parent().GlobalTransformation().Inverse() * globalTransform;
        } else {
            comp.localTransformation = globalTransform;
        }

        UpdateTransformations();
    }

    void SetLocalTransformation(const Transformation& localTransform) {
        auto& comp{ Component<TransformationComponent>() };

        comp.localTransformation = localTransform;
        if (Parent()) {
            comp.globalTransformation = Parent().GlobalTransformation() * localTransform;
        } else {
            comp.globalTransformation = localTransform;
        }

        UpdateTransformations();
    }

    GameObject Clone();

    UGINE_FORCE_INLINE glm::mat4 ViewMatrix() const { return glm::inverse(GlobalTransformation().Matrix()); }

private:
    void UpdateTransformations() {
        const auto& comp{ Component<TransformationComponent>() };

        handle_.patch<TransformationComponent>([](auto&) {});
        for (auto c{ FirstChild() }; c.IsValid(); c = c.NextSibling()) {
            c.SetGlobalTransformation(comp.globalTransformation * c.LocalTransformation());
        }
    }

    GameObject CloneImpl(bool isRoot);

    entt::handle handle_;
};

bool operator==(const GameObject& a, const GameObject& b);
bool operator!=(const GameObject& a, const GameObject& b);

} // namespace ugine
