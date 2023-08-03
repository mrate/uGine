#include "GameObject.h"

#include <ugine/engine/world/World.h>

#include <ugine/engine/gfx/Component.h>
#include <ugine/engine/script/Component.h>
#include <ugine/engine/script/Lua.h>

namespace ugine {

namespace {
    template <typename T> void Copy(const GameObject& src, GameObject& dst) {
        if (src.Has<T>()) {
            dst.CreateComponent<T>(src.Component<T>());
        }
    }
} // namespace

GameObject GameObject::Create(GameObjectRegistry& registry, std::string_view name) {
    auto go{ registry.create() };
    registry.emplace<TagComponent>(go, TagComponent::Init(name));
    registry.emplace<RelationshipComponent>(go);
    registry.emplace<TransformationComponent>(go);

    return GameObject{ registry, go };
}

void GameObject::Destroy(const GameObject& go) {
    for (auto child{ go.FirstChild() }; child.IsValid();) {
        auto tmp{ child };
        child = child.NextSibling();

        if (tmp.Parent().IsValid()) {
            tmp.Parent().RemoveChild(tmp);
        }

        Destroy(tmp);
    }

    if (go.Parent().IsValid()) {
        go.Parent().RemoveChild(go);
    }

    go.handle_.registry()->destroy(go.Entity());
}

void GameObject::AddChild(GameObjectHandle child) {
    auto childGO{ World()->Get(child) };

    auto& rel{ Component<RelationshipComponent>() };
    auto& childRel{ childGO.Component<RelationshipComponent>() };
    auto childGlob{ childGO.GlobalTransformation() };

    UGINE_ASSERT(handle_.registry()->try_get<ParentComponent>(child) == nullptr);
    UGINE_ASSERT(childRel.prevSibling == GameObjectNull);
    UGINE_ASSERT(childRel.nextSibling == GameObjectNull);

    handle_.registry()->emplace<ParentComponent>(child, Entity());

    if (rel.children == 0) {
        rel.firstChild = rel.lastChild = child;
    } else {
        auto& lastChildRel{ handle_.registry()->get<RelationshipComponent>(rel.lastChild) };
        childRel.prevSibling = rel.lastChild;
        lastChildRel.nextSibling = child;
        rel.lastChild = child;
    }

    ++rel.children;

    childGO.SetGlobalTransformation(childGlob);
}

void GameObject::RemoveChild(GameObjectHandle child) {
    auto childGO{ World()->Get(child) };

    auto& rel{ Component<RelationshipComponent>() };
    auto& childRel{ childGO.Component<RelationshipComponent>() };

    auto childGlob{ childGO.GlobalTransformation() };

    UGINE_ASSERT(handle_.registry()->get<ParentComponent>(child).parent == Entity());
    UGINE_ASSERT(rel.children > 0);

    if (rel.firstChild == child) {
        rel.firstChild = childRel.nextSibling;
    }

    if (rel.lastChild == child) {
        rel.lastChild = childRel.prevSibling;
    }

    if (childRel.prevSibling != GameObjectNull) {
        auto& prevRel{ handle_.registry()->get<RelationshipComponent>(childRel.prevSibling) };
        prevRel.nextSibling = childRel.nextSibling;
    }

    if (childRel.nextSibling != GameObjectNull) {
        auto& nextRel{ handle_.registry()->get<RelationshipComponent>(childRel.nextSibling) };
        nextRel.prevSibling = childRel.prevSibling;
    }

    childRel.prevSibling = childRel.nextSibling = GameObjectNull;
    childGO.handle_.remove<ParentComponent>();

    --rel.children;

    childGO.SetGlobalTransformation(childGlob);
}

GameObject GameObject::Clone() {
    return CloneImpl(true);
}

GameObject GameObject::CloneImpl(bool isRoot) {
    auto newGo{ World()->CreateObject(Name()) };
    newGo.SetGlobalTransformation(GlobalTransformation());

    // Copy components.
    Copy<CameraComponent>(*this, newGo);
    Copy<MeshComponent>(*this, newGo);
    Copy<AnimationControllerComponent>(*this, newGo);
    Copy<SkyComponent>(*this, newGo);
    Copy<LightComponent>(*this, newGo);
    Copy<LuaScriptComponent>(*this, newGo);

    for (auto child{ FirstChild() }; child.IsValid(); child = child.NextSibling()) {
        newGo.AddChild(child.CloneImpl(false));
    }

    if (isRoot) {
        auto parent{ Parent() };
        if (parent) {
            parent.AddChild(newGo);
        }
    }

    return newGo;
}

bool operator==(const GameObject& a, const GameObject& b) {
    return (!a && !b) || (a.Entity() == b.Entity());
}

bool operator!=(const GameObject& a, const GameObject& b) {
    return !(a == b);
}

} // namespace ugine
