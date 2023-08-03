#include <format>
#include <iostream>

#include <lua.hpp>

#include <LuaBridge/LuaBridge.h>

#include <glm/glm.hpp>

using namespace luabridge;

struct MyStruct {
    int member{};
};

struct Entity {
    Entity() = default;

    std::string name;

    int GetComponent() { return int(name.size()); }

    // Internal

    std::unique_ptr<LuaRef> script;
    std::string scriptName;

    void CallScript() {
        try {
            (*script)(this);
        } catch (const std::exception& ex) {
            std::cerr << ex.what() << std::endl;
        }
    }
};

class Script {
public:
    Script() {
        L = luaL_newstate();
        luaL_openlibs(L);

        getGlobalNamespace(L)                               //
            .beginNamespace("ugine")                        //
            .beginClass<MyStruct>("MyStruct")               //
            .addConstructor<void (*)(void)>()               //
            .addProperty("member", &MyStruct::member, true) //
            .endClass()                                     //

            .beginClass<Entity>("Entity")
            .addProperty("name", &Entity::name)
            .addFunction("getComponent", &Entity::GetComponent)
            .endClass()

            .beginClass<glm::vec3>("vec3")
            .addProperty("x", &glm::vec3::x, true)
            .addProperty("y", &glm::vec3::y, true)
            .addProperty("z", &glm::vec3::z, true)
            .endClass()
            .addFunction(
                "__mul", std::function<glm::vec3(const glm::vec3& a, const glm::vec3& b)>{ [](const glm::vec3& a, const glm::vec3& b) { return a * b; } })
            .addFunction("__mul", std::function<glm::vec3(const glm::vec3& a, float b)>{ [](const glm::vec3& a, float b) { return a * b; } })

            .endNamespace(); //
    }

    ~Script() { lua_close(L); }

    void Load(std::string_view script) {
        if (luaL_loadstring(L, script.data()) != 0) {
            throw std::runtime_error{ lua_tostring(L, -1) };
        }

        if (lua_pcall(L, 0, 0, -2) != 0) {
            throw std::runtime_error{ lua_tostring(L, -1) };
        }
    }

    template <typename... Args> void Call(std::string_view func, Args&&... args) {
        LuaRef ref{ getGlobal(L, func.data()) };
        if (!ref) {
            throw std::runtime_error{ std::format("Function '{}' not found", func) };
        }
        ref(std::forward<Args&&>(args)...);
    }

    lua_State* L{};
};

const char* source = R"script(
function Update(struct, vec)
    print("Hello, I've got ", struct.member, vec.x);

    vec = vec * 5.0;

    s = ugine.MyStruct();
    s.member = 5;

    struct.member = 666;
end
)script";

void test1() {
    try {
        Script script{};

        script.Load(source);

        MyStruct value{ 5 };
        glm::vec3 vec{ 1.0f, 2.0f, 3.0f };

        script.Call("Update", &value, &vec);

        std::cout << std::format("Back in C++: {}\n", value.member);
    } catch (const std::exception& ex) {
        std::cerr << std::format("Script error: {}\n", ex.what());
    }
}

const char* sourceMod1 = R"script(
local mymodule = {}

function mymodule.update(entity)
    print("I'm a ", entity.name)
end

return mymodule
)script";

const char* sourceMod2 = R"script(
local mymodule = {}

function mymodule.update2(entity)
    print("I'm other ", entity.name)
end

return mymodule
)script";

void AddScript(lua_State* state, Entity& entity, std::string_view script) {
    static uint64_t counter{};

    entity.scriptName = std::format("script_{}", counter++);

    LuaRef load = getGlobal(state, "load");
    auto loadRes{ load(script.data()) };
    assert(loadRes.wasOk());
    setGlobal(state, loadRes[0](), entity.scriptName.c_str());
    entity.script = std::make_unique<LuaRef>(getGlobal(state, entity.scriptName.c_str())["update"]);
}

void testModule() {
    try {
        Script script{};

        //script.Load("newObject.foo()");
        Entity ent1;
        Entity ent2;

        ent1.name = "some entity";
        ent2.name = "other entity";

        AddScript(script.L, ent1, sourceMod1);
        AddScript(script.L, ent2, sourceMod2);
        // TODO:

        ent1.CallScript();
        ent2.CallScript();

    } catch (const std::exception& ex) {
        std::cerr << std::format("Script error: {}\n", ex.what());
    }
}

struct ComponentA {
    int value{ 666 };
};

struct ComponentB {
    std::string name{ "CompB" };
};

template <typename T> T Create() {
    return T{};
}

const auto templateSrc = R"script(
local a = test.CreateA()
print(a.value)

local b = test.CreateB()
print(b.name)
)script";

void testTemplates() {
    try {
        Script script{};

        getGlobalNamespace(script.L)
            .beginNamespace("test")
            .beginClass<ComponentA>("ComponentA")
            .addProperty("value", &ComponentA::value)
            .endClass()
            .beginClass<ComponentB>("ComponentB")
            .addProperty("name", &ComponentB::name)
            .endClass()
            .addFunction("CreateA", std::function<ComponentA()>([] { return Create<ComponentA>(); }))
            .addFunction("CreateB", std::function<ComponentB()>([] { return Create<ComponentB>(); }))
            .endNamespace();

        script.Load(templateSrc);

    } catch (const std::exception& ex) {
        std::cerr << std::format("Script error: {}\n", ex.what());
    }
}

int main(int argc, char* argv[]) {

    //testModule();
    testTemplates();

    return 0;
}