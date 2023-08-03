#include "Lua.h"

#include <ugine/engine/engine/Engine.h>
#include <ugine/engine/gfx/Component.h>
#include <ugine/engine/gfx/GraphicsScene.h>
#include <ugine/engine/gfx/helpers/DebugRenderer.h>
#include <ugine/engine/input/InputState.h>
#include <ugine/engine/script/Component.h>
#include <ugine/engine/script/LuaScript.h>
#include <ugine/engine/world/GameObject.h>
#include <ugine/engine/world/Transformation.h>
#include <ugine/engine/world/World.h>
#include <ugine/engine/world/WorldManager.h>
#include <ugine/engine/world/WorldSerializer.h>

#include <LuaBridge/Array.h>
#include <LuaBridge/Vector.h>

template <> struct luabridge::Stack<ugine::InputKeyCode> : luabridge::Enum<ugine::InputKeyCode> {};

namespace ugine {

std::string StackToString(lua_State* state) {
    std::stringstream str;
    const auto top{ lua_gettop(state) };
    for (int i{ 1 }; i <= top; ++i) {
        const auto type{ lua_type(state, i) };
        switch (type) {
        case LUA_TSTRING: str << lua_tostring(state, i); break;
        case LUA_TBOOLEAN: str << lua_toboolean(state, i); break;
        case LUA_TNUMBER: str << lua_tonumber(state, i); break;
        case LUA_TTABLE: str << "[table]"; break;
        case LUA_TFUNCTION: str << "[function]"; break;
        case LUA_TUSERDATA: str << "[userdata]"; break;
        case LUA_TTHREAD: str << "[thread]"; break;
        case LUA_TLIGHTUSERDATA: str << "[lightuserdata]"; break;
        case LUA_TNIL: str << "[nil]"; break;
        default: str << "[n/a]"; break;
        }
    }
    return str.str();
}

void LogTrace(lua_State* state) {
    const auto msg{ StackToString(state) };
    UGINE_TRACE(msg);
}

void LogDebug(lua_State* state) {
    const auto msg{ StackToString(state) };
    UGINE_DEBUG(msg);
}

void LogInfo(lua_State* state) {
    const auto msg{ StackToString(state) };
    UGINE_INFO(msg);
}

void LogWarn(lua_State* state) {
    const auto msg{ StackToString(state) };
    UGINE_WARN(msg);
}

void LogError(lua_State* state) {
    const auto msg{ StackToString(state) };
    UGINE_ERROR(msg);
}

// Can't modify components directly from script, need to call GameObject::Patch.
template <typename T> struct ComponentWrapper {
    GameObject owner;
    T& component;
};

template <typename T> ComponentWrapper<T> CreateComponentWrapper(GameObject& go) {
    return ComponentWrapper<T>{ go, go.CreateComponent<T>() };
}

template <typename T> ComponentWrapper<T> GetComponentWrapper(GameObject& go) {
    return ComponentWrapper<T>{ go, go.Component<T>() };
}

void RegisterLuaState(lua_State* state) {
    using namespace luabridge;

#define DECLARE_RESOURCE(TYPE)                                                                                                                                 \
    .deriveClass<TYPE, Resource>(#TYPE)                                                                                                                        \
        .endClass()                                                                                                                                            \
        .deriveClass<ResourceHandle<TYPE>, ResourceHandleTypeless>("Resource" #TYPE)                                                                           \
        .addFunction("get", static_cast<TYPE* (ResourceHandle<TYPE>::*)()>(&ResourceHandle<TYPE>::Get))                                                        \
        .endClass()

#define DECLARE_COMPONENT(TYPE)                                                                                                                                \
    .addFunction("create" #TYPE, &CreateComponentWrapper<TYPE>)                                                                                                \
        .addFunction("destroy" #TYPE, &GameObject::RemoveComponent<TYPE>)                                                                                      \
        .addFunction("get" #TYPE, GetComponentWrapper<TYPE>)                                                                                                   \
        .addFunction("has" #TYPE, &GameObject::Has<TYPE>)

#define BEGIN_COMPONENT(TYPE) .beginClass<ComponentWrapper<TYPE>>(#TYPE)
#define END_COMPONENT() .endClass()
#define COMPONENT_PROPERTY(TYPE, NAME)                                                                                                                         \
    .addProperty(                                                                                                                                              \
        #NAME, +[](const ComponentWrapper<TYPE>* c) { return c->component.NAME; },                                                                             \
        +[](ComponentWrapper<TYPE>* c, decltype(TYPE::NAME) value) { c->owner.Patch<TYPE>([&](auto& comp) { comp.NAME = value; }); })

    getGlobalNamespace(state)
        .beginNamespace("ugine")

        .beginClass<glm::vec3>("Vec3")
        .addConstructor<void(), void(float, float, float)>()
        .addProperty("x", &glm::vec3::x)
        .addProperty("y", &glm::vec3::y)
        .addProperty("z", &glm::vec3::z)
        .addFunction(
            "__add", +[](const glm::vec3* v, const glm::vec3* b) { return *v + *b; }, +[](const glm::vec3* v, float b) { return *v + b; })
        .addFunction(
            "__sub", +[](const glm::vec3* v, const glm::vec3* b) { return *v - *b; }, +[](const glm::vec3* v, float b) { return *v - b; })
        .addFunction(
            "__mul", +[](const glm::vec3* v, const glm::vec3* b) { return *v * *b; }, +[](const glm::vec3* v, float b) { return *v * b; })
        .addFunction(
            "__div", +[](const glm::vec3* v, const glm::vec3* b) { return *v / *b; }, +[](const glm::vec3* v, float b) { return *v / b; })
        .addFunction(
            "__tostring", +[](const glm::vec3* v) { return std::format("Vec3[{},{},{}]", v->x, v->y, v->z); })
        .endClass()

        .beginClass<glm::fquat>("Quat")
        .addConstructor<void(), void(float, float, float, float)>()
        .addStaticFunction(
            "angleAxis", +[](float angle, const glm::vec3& axis) { return glm::angleAxis(angle, axis); })
        .addFunction(
            "eulerAngles", +[](glm::fquat* quat) { return glm::eulerAngles(*quat); })
        .addProperty("x", &glm::fquat::x)
        .addProperty("y", &glm::fquat::y)
        .addProperty("z", &glm::fquat::z)
        .addProperty("w", &glm::fquat::w)
        .addFunction(
            "__mul", +[](const glm::fquat* v, const glm::vec3* b) { return *v * *b; }, +[](const glm::fquat* v, const glm::fquat* b) { return *v * *b; })
        .addFunction(
            "__tostring", +[](const glm::fquat* v) { return std::format("Quat[{},{},{},{}]", v->x, v->y, v->z, v->w); })
        .endClass()

        .beginClass<StringID>("StringID")
        .addConstructor<void(), void(const char*)>()
        .addProperty("value", &StringID::Value)
        .endClass()

        .addFunction("trace", LogTrace) //
        .addFunction("debug", LogDebug) //
        .addFunction("info", LogInfo)   //
        .addFunction("warn", LogWarn)   //
        .addFunction("error", LogError)

        // Core
        .beginClass<InputState>("InputState")
        .addFunction("getAction", &InputState::GetAction)
        .addFunction("keyDown", &InputState::KeyDown)
        .addFunction("keyPressed", &InputState::KeyPressed)
        .endClass()

        .addProperty(
            "Key_Escape", +[] { return InputKeyCode::Key_Escape; })
        .addProperty(
            "Key_Tab", +[] { return InputKeyCode::Key_Tab; })
        .addProperty(
            "Key_A", +[] { return InputKeyCode::Key_A; })
        .addProperty(
            "Key_B", +[] { return InputKeyCode::Key_B; })
        .addProperty(
            "Key_C", +[] { return InputKeyCode::Key_C; })
        .addProperty(
            "Key_D", +[] { return InputKeyCode::Key_D; })
        .addProperty(
            "Key_E", +[] { return InputKeyCode::Key_E; })
        .addProperty(
            "Key_F", +[] { return InputKeyCode::Key_F; })
        .addProperty(
            "Key_G", +[] { return InputKeyCode::Key_G; })
        .addProperty(
            "Key_H", +[] { return InputKeyCode::Key_H; })
        .addProperty(
            "Key_I", +[] { return InputKeyCode::Key_I; })
        .addProperty(
            "Key_J", +[] { return InputKeyCode::Key_J; })
        .addProperty(
            "Key_K", +[] { return InputKeyCode::Key_K; })
        .addProperty(
            "Key_L", +[] { return InputKeyCode::Key_L; })
        .addProperty(
            "Key_M", +[] { return InputKeyCode::Key_M; })
        .addProperty(
            "Key_N", +[] { return InputKeyCode::Key_N; })
        .addProperty(
            "Key_O", +[] { return InputKeyCode::Key_O; })
        .addProperty(
            "Key_P", +[] { return InputKeyCode::Key_P; })
        .addProperty(
            "Key_Q", +[] { return InputKeyCode::Key_Q; })
        .addProperty(
            "Key_R", +[] { return InputKeyCode::Key_R; })
        .addProperty(
            "Key_S", +[] { return InputKeyCode::Key_S; })
        .addProperty(
            "Key_T", +[] { return InputKeyCode::Key_T; })
        .addProperty(
            "Key_U", +[] { return InputKeyCode::Key_U; })
        .addProperty(
            "Key_V", +[] { return InputKeyCode::Key_V; })
        .addProperty(
            "Key_W", +[] { return InputKeyCode::Key_W; })
        .addProperty(
            "Key_X", +[] { return InputKeyCode::Key_X; })
        .addProperty(
            "Key_Y", +[] { return InputKeyCode::Key_Y; })
        .addProperty(
            "Key_Z", +[] { return InputKeyCode::Key_Z; })
        .addProperty(
            "Key_0", +[] { return InputKeyCode::Key_0; })
        .addProperty(
            "Key_1", +[] { return InputKeyCode::Key_1; })
        .addProperty(
            "Key_2", +[] { return InputKeyCode::Key_2; })
        .addProperty(
            "Key_3", +[] { return InputKeyCode::Key_3; })
        .addProperty(
            "Key_4", +[] { return InputKeyCode::Key_4; })
        .addProperty(
            "Key_5", +[] { return InputKeyCode::Key_5; })
        .addProperty(
            "Key_6", +[] { return InputKeyCode::Key_6; })
        .addProperty(
            "Key_7", +[] { return InputKeyCode::Key_7; })
        .addProperty(
            "Key_8", +[] { return InputKeyCode::Key_8; })
        .addProperty(
            "Key_9", +[] { return InputKeyCode::Key_9; })
        .addProperty(
            "Key_F1", +[] { return InputKeyCode::Key_F1; })
        .addProperty(
            "Key_F2", +[] { return InputKeyCode::Key_F2; })
        .addProperty(
            "Key_F3", +[] { return InputKeyCode::Key_F3; })
        .addProperty(
            "Key_F4", +[] { return InputKeyCode::Key_F4; })
        .addProperty(
            "Key_F5", +[] { return InputKeyCode::Key_F5; })
        .addProperty(
            "Key_F6", +[] { return InputKeyCode::Key_F6; })
        .addProperty(
            "Key_F7", +[] { return InputKeyCode::Key_F7; })
        .addProperty(
            "Key_F8", +[] { return InputKeyCode::Key_F8; })
        .addProperty(
            "Key_F9", +[] { return InputKeyCode::Key_F9; })
        .addProperty(
            "Key_F10", +[] { return InputKeyCode::Key_F10; })
        .addProperty(
            "Key_F11", +[] { return InputKeyCode::Key_F11; })
        .addProperty(
            "Key_F12", +[] { return InputKeyCode::Key_F12; })
        .addProperty(
            "Key_Shift", +[] { return InputKeyCode::Key_Shift; })

        .beginClass<Engine>("Engine")
        .addProperty("frameNumber", &Engine::FrameNumber)
        .addProperty("seconds", &Engine::Seconds)
        .addProperty("millis", &Engine::Millis)
        .addProperty("frameSeconds", &Engine::FrameSeconds)
        .addProperty("frameMillis", &Engine::FrameMillis)
        .addProperty("secondsDelta", &Engine::FrameSecondsDelta)
        .addProperty("millisDelta", &Engine::FrameMillisDelta)
        .addFunction("getWorlds", &Engine::GetWorldManager)
        .addFunction("getResources", &Engine::GetResources)
        .addFunction("quit", &Engine::Quit)
        .addFunction(
            "getInput", +[](Engine* engine) { return engine->GetState<InputState>(); })
        .endClass()

        .beginClass<World>("World")                        //
        .addFunction("createObject", &World::CreateObject) //
        .addFunction("findAll", &World::Find)
        .addFunction(
            "find",
            +[](World& world, std::string_view name) {
                auto object{ world.Find(name) };
                return object.empty() ? GameObject{} : object[0];
            })
        .addFunction(
            "debugRenderer", +[](World* world) { return &world->GetScene<GraphicsScene>()->GetDebugRenderer(); })
        .endClass()

        .beginClass<WorldManager>("WorldManager")         //
        .addFunction("getWorld", &WorldManager::GetWorld) //
        .addFunction("size", &WorldManager::Size)         //
        .addFunction("createWorld", &WorldManager::CreateWorld)
        .addFunction("destroyWorld", &WorldManager::DestroyWorld)
        .addFunction("getDefaultWorld", &WorldManager::GetDefaultWorld)
        .addFunction("setDefaultWorld", &WorldManager::SetDefaultWorld)
        .addFunction(
            "loadWorld",
            +[](WorldManager& wm, std::string_view path) {
                // TODO:
                auto defaultWorld{ wm.GetDefaultWorld() };
                auto newWorld{ wm.CreateWorld() };
                WorldSerializer ser{};
                ser.Deserialize(*newWorld, wm.GetEngine(), Path{ path.data() });

                if (defaultWorld) {
                    wm.DestroyWorld(defaultWorld);
                }
                wm.SetDefaultWorld(newWorld);
            })
        .endClass()

        .beginClass<Transformation>("Transformation")
        .addProperty("position", &Transformation::position)
        .addProperty("rotation", &Transformation::rotation)
        .addProperty("scale", &Transformation::scale)
        .endClass()

        .beginClass<GameObject>("GameObject")
        .addProperty("name", &GameObject::Name, &GameObject::SetName)
        .addProperty( // Return by value
            "localTransform", +[](const GameObject* go) { return Transformation{ go->LocalTransformation() }; },
            +[](GameObject* go, const Transformation& t) { go->SetLocalTransformation(t); })
        .addProperty( // Return by value
            "globalTransform", +[](const GameObject* go) { return Transformation{ go->GlobalTransformation() }; },
            +[](GameObject* go, const Transformation& t) { go->SetGlobalTransformation(t); })
        .addProperty("enabled", &GameObject::IsEnabled, &GameObject::SetEnabled)
        .addProperty("static", &GameObject::IsStatic, &GameObject::SetStatic)
        .addProperty(
            "world", +[](const GameObject* go) { return go->World(); })
        .addProperty("parent", &GameObject::Parent)
        .addProperty("isValid", &GameObject::IsValid)

            DECLARE_COMPONENT(MeshComponent)            //
        DECLARE_COMPONENT(SkyComponent)                 //
        DECLARE_COMPONENT(LightComponent)               //
        DECLARE_COMPONENT(AnimationControllerComponent) //
        DECLARE_COMPONENT(LuaScriptComponent)           //

        .endClass()

        // TODO:
        .beginClass<Path>("Path")
        .endClass()

        // Resources

        .beginClass<ResourceID>("ResourceID")
        .addFunction("isNull", &ResourceID::IsNull)
        .addFunction(
            "__tostring", +[](const ResourceID& id) { return std::string{ id.ToString().Data() }; })
        .endClass()

        .beginClass<ResourceType>("ResourceType")
        .addProperty("name", &ResourceType::Name)
        .addFunction("__tostring", &ResourceType::Name)
        .endClass()

        .beginClass<Resource>("Resource")
        .addProperty("id", &Resource::Id)
        .addProperty("type", &Resource::Type)
        .addProperty("ready", &Resource::Ready)
        .endClass()

        .beginClass<ResourceHandleTypeless>("ResourceHandle")
        .addFunction(
            "getResource", +[](ResourceHandleTypeless& handle) { return handle.Get(); })
        .endClass()

            DECLARE_RESOURCE(Model) DECLARE_RESOURCE(Material) DECLARE_RESOURCE(Texture) DECLARE_RESOURCE(Animation) DECLARE_RESOURCE(LuaScript)

        .beginClass<ResourceManager>("ResourceManager")
        .addFunction(
            "findResource", +[](ResourceManager& res, std::string_view name) { return res.FindResource(Path{ name.data() }); }) // Convert string_view to path.
        .addFunction("resourceType", &ResourceManager::ResourceType)
        .addFunction(
            "resourcePath", +[](ResourceManager& res, const ResourceID& id) { return std::string{ res.ResourcePath(id).Data() }; }) // TODO:
        .addFunction("getModel", &ResourceManager::Get<Model>)
        .addFunction("getMaterial", &ResourceManager::Get<Material>)
        .addFunction("getTexture", &ResourceManager::Get<Texture>)
        .addFunction("getAnimation", &ResourceManager::Get<Animation>)
        .addFunction("getLuaScript", &ResourceManager::Get<LuaScript>)
        .endClass()

        .beginClass<ColorRGB>("ColorRGB")
        .addProperty("r", &ColorRGB::r)
        .addProperty("g", &ColorRGB::g)
        .addProperty("b", &ColorRGB::b)
        .endClass()

        .beginClass<ColorRGBA>("ColorRGBA")
        .addProperty("r", &ColorRGBA::r)
        .addProperty("g", &ColorRGBA::g)
        .addProperty("b", &ColorRGBA::b)
        .addProperty("a", &ColorRGBA::a)
        .endClass()

        // Graphics

        .beginClass<DebugRenderer>("DebugRenderer")
        .addFunction("drawLine", &DebugRenderer::AddLine)
        .addFunction("drawCube", &DebugRenderer::AddCube)
        .addFunction("drawTriangle", &DebugRenderer::AddTriangle)
        .addFunction("drawSphere", &DebugRenderer::AddSphere)
        .addFunction("drawCircle", &DebugRenderer::AddCircle)
        .addFunction("drawCuboid", &DebugRenderer::AddCuboid)
        .endClass()

        // Components

        BEGIN_COMPONENT(MeshComponent) //
        .addFunction(
            "setModel",
            +[](ComponentWrapper<MeshComponent>& c, const ResourceHandle<Model>& model) {
                c.owner.Patch<MeshComponent>([&](auto& c) { c.modelInstance.SetModel(model); });
            }) //
        .addFunction(
            "setMaterial",
            +[](ComponentWrapper<MeshComponent>& c, int index, const ResourceHandle<Material>& material) {
                c.owner.Patch<MeshComponent>([&](auto& c) { c.modelInstance.SetMaterial(index, material); });
            }) //
        .addFunction(
            "resetMaterials",
            +[](ComponentWrapper<MeshComponent>& c, int index, const ResourceHandle<Material>& material) {
                c.owner.Patch<MeshComponent>([&](auto& c) { c.modelInstance.ResetMaterials(); });
            }) //
        COMPONENT_PROPERTY(MeshComponent, instanced)
        .addProperty(
            "instanceCount", +[](const ComponentWrapper<MeshComponent>* c) { return c->component.instanceTransformations.size(); },
            +[](ComponentWrapper<MeshComponent>* c, int count) {
                c->owner.Patch<MeshComponent>([&](auto& comp) { comp.instanceTransformations.resize(count); });
            })
        .addFunction(
            "getInstanceTransformation",
            +[](ComponentWrapper<MeshComponent>& c, int index) { return Transformation{ ToMatrix(c.component.instanceTransformations[index]) }; })
        .addFunction(
            "setInstanceTransformation",
            +[](ComponentWrapper<MeshComponent>& c, int index, const Transformation& trans) {
                c.owner.Patch<MeshComponent>([&](auto& comp) { comp.instanceTransformations[index] = MaterialVertexInstanceFromMatrix(trans.Matrix()); });
            })          //
        END_COMPONENT() //

        BEGIN_COMPONENT(AnimationControllerComponent)                   //
        COMPONENT_PROPERTY(AnimationControllerComponent, animation)     //
        COMPONENT_PROPERTY(AnimationControllerComponent, isRunning)     //
        COMPONENT_PROPERTY(AnimationControllerComponent, speed)         //
        COMPONENT_PROPERTY(AnimationControllerComponent, resolutionS)   //
        COMPONENT_PROPERTY(AnimationControllerComponent, animationTime) //
        END_COMPONENT()                                                 //

        BEGIN_COMPONENT(LightComponent)                  //
        COMPONENT_PROPERTY(LightComponent, intensity)    //
        COMPONENT_PROPERTY(LightComponent, color)        //
        COMPONENT_PROPERTY(LightComponent, range)        //
        COMPONENT_PROPERTY(LightComponent, spotAngleDeg) //
        END_COMPONENT()                                  //

        BEGIN_COMPONENT(SkyComponent)              //
        COMPONENT_PROPERTY(SkyComponent, material) //
        END_COMPONENT()                            //

        BEGIN_COMPONENT(LuaScriptComponent)            //
        COMPONENT_PROPERTY(LuaScriptComponent, script) //
        END_COMPONENT()                                //

        //

        .endNamespace();
}

} // namespace ugine