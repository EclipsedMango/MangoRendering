
#include "ScriptManager.h"

#include "LuaIncludes.hpp"
#include <sol/sol.hpp>
#include <iostream>
#include <utility>
#include <variant>
#include <tracy/Tracy.hpp>

#include "Input.h"
#include "ResourceManager.h"
#include "Nodes/CameraNode3d.h"
#include "Nodes/Node3d.h"
#include "Nodes/RigidBody3d.h"

struct ScriptManager::Impl {
    struct ScriptInstance {
        sol::table table;
        sol::protected_function init;
        sol::protected_function ready;
        sol::protected_function process;
        sol::protected_function physicsProcess;
        bool initialized = false;
    };

    sol::state lua;
    std::unordered_map<Node3d*, ScriptInstance> scripts;
    bool runtimeEnabled = true;
    std::function<void()> quitHandler;
};

ScriptManager::ScriptManager() : m_impl(std::make_unique<Impl>()) {}
ScriptManager::~ScriptManager() = default;

static void ReportProtectedFunctionError(const sol::protected_function_result& result, const std::string& context) {
    if (result.valid()) {
        return;
    }

    const sol::error err = result;
    std::cerr << "[Lua Error] " << context << ":\n" << err.what() << std::endl;
}

static sol::object PropertyValueToLua(const PropertyValue& val, const sol::state_view &lua) {
    if (std::holds_alternative<int>(val)) { return sol::make_object(lua, std::get<int>(val)); }
    if (std::holds_alternative<float>(val)) { return sol::make_object(lua, std::get<float>(val)); }
    if (std::holds_alternative<bool>(val)) { return sol::make_object(lua, std::get<bool>(val)); }
    if (std::holds_alternative<std::string>(val)) { return sol::make_object(lua, std::get<std::string>(val)); }
    if (std::holds_alternative<glm::vec2>(val)) { return sol::make_object(lua, std::get<glm::vec2>(val)); }
    if (std::holds_alternative<glm::vec3>(val)) { return sol::make_object(lua, std::get<glm::vec3>(val)); }
    if (std::holds_alternative<std::shared_ptr<PropertyHolder>>(val)) {
        auto ptr = std::get<std::shared_ptr<PropertyHolder>>(val);
        if (ptr) return sol::make_object(lua, ptr);
        return sol::lua_nil;
    }

    return sol::make_object(lua, sol::lua_nil);
}

static PropertyValue LuaToPropertyValue(const sol::object& luaVal) {
    if (luaVal.is<bool>()) return luaVal.as<bool>();
    if (luaVal.is<std::string>()) return luaVal.as<std::string>();
    if (luaVal.is<glm::vec2>()) return luaVal.as<glm::vec2>();
    if (luaVal.is<glm::vec3>()) return luaVal.as<glm::vec3>();
    if (luaVal.is<std::shared_ptr<PropertyHolder>>()) return luaVal.as<std::shared_ptr<PropertyHolder>>();

    if (luaVal.is<double>()) {
        const double val = luaVal.as<double>();
        if (std::floor(val) == val && val >= std::numeric_limits<int>::min() && val <= std::numeric_limits<int>::max()) {
            return static_cast<float>(val);
        }
        return static_cast<float>(val);
    }

    throw std::runtime_error("Unsupported Lua type passed to PropertyValue");
}


void ScriptManager::Init() {
    auto& lua = m_impl->lua;

    lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::math, sol::lib::string, sol::lib::table);
    lua.set_panic([](lua_State* L) -> int {
        const char* msg = lua_tostring(L, -1);
        std::cerr << "[Lua Panic Error] " << (msg ? msg : "Unknown Error") << std::endl;
        return 0;
    });

    lua.new_usertype<glm::vec2>("vec2",
        sol::constructors<glm::vec2(), glm::vec2(float), glm::vec2(float, float)>(),
        "x", &glm::vec2::x,
        "y", &glm::vec2::y,
        sol::meta_function::addition, [](const glm::vec2& a, const glm::vec2& b) { return a + b; },
        sol::meta_function::subtraction, [](const glm::vec2& a, const glm::vec2& b) { return a - b; },
        sol::meta_function::multiplication, [](const glm::vec2& a, const float scalar) { return a * scalar; },
        sol::meta_function::division, [](const glm::vec2& a, const float scalar) { return a / scalar; }
    );

    lua.new_usertype<glm::vec3>("vec3",
        sol::constructors<glm::vec3(), glm::vec3(float), glm::vec3(float, float, float)>(),
        "x", &glm::vec3::x,
        "y", &glm::vec3::y,
        "z", &glm::vec3::z,
        sol::meta_function::addition, [](const glm::vec3& a, const glm::vec3& b) { return a + b; },
        sol::meta_function::subtraction, [](const glm::vec3& a, const glm::vec3& b) { return a - b; },
        sol::meta_function::multiplication, [](const glm::vec3& a, const float scalar) { return a * scalar; },
        sol::meta_function::division, [](const glm::vec3& a, const float scalar) { return a / scalar; }
    );

    lua.new_usertype<PropertyHolder>("PropertyHolder",
        "Get", [](const PropertyHolder* holder, const std::string& propName, const sol::this_state s) -> sol::object {
            const sol::state_view lua2(s);
            try {
                const PropertyValue val = holder->GetProperty(propName);
                return PropertyValueToLua(val, lua2);
            } catch(const std::exception& e) {
                std::cerr << "[Lua Warning] Failed to get nested property '" << propName << "': " << e.what() << "\n";
                return sol::make_object(lua2, sol::lua_nil);
            }
        },
        "Set", [](PropertyHolder* holder, const std::string& propName, const sol::object& luaVal) {
            try {
                const PropertyValue val = LuaToPropertyValue(luaVal);
                holder->Set(propName, val);
            } catch(const std::exception& e) {
                std::cerr << "[Lua Warning] Failed to set nested property '" << propName << "': " << e.what() << "\n";
            }
        }
    );

    lua.new_usertype<Node3d>("Node3d",
        sol::base_classes, sol::bases<PropertyHolder>(),
        "GetName", &Node3d::GetName,
        "SetName", &Node3d::SetName,
        "GetPosition", &Node3d::GetPosition,
        "SetPosition", &Node3d::SetPosition,
        "GetRotationEuler", &Node3d::GetRotationEuler,
        "SetRotationEuler", &Node3d::SetRotationEuler,
        "GetScale", &Node3d::GetScale,
        "SetScale", &Node3d::SetScale,
        "IsVisible", &Node3d::IsVisible,
        "SetVisible", &Node3d::SetVisible,
        "GetParent", &Node3d::GetParent,
        "GetNodeType", &Node3d::GetNodeType,
        "AsCamera", [](Node3d* node) -> CameraNode3d* {
            return dynamic_cast<CameraNode3d*>(node);
        },
        "AsRigidBody", [](Node3d* node) -> RigidBody3d* {
            return dynamic_cast<RigidBody3d*>(node);
        }
    );

    lua.new_usertype<CameraNode3d>("CameraNode3d",
        sol::base_classes, sol::bases<Node3d, PropertyHolder>(),
        "Rotate", &CameraNode3d::Rotate,
        "GetFront", &CameraNode3d::GetFront,
        "GetRight", &CameraNode3d::GetRight,
        "GetUp", &CameraNode3d::GetUp
    );

    lua.new_usertype<RigidBody3d>("RigidBody3d",
        sol::base_classes, sol::bases<Node3d, PropertyHolder>(),
        "GetLinearVelocity", &RigidBody3d::GetLinearVelocity,
        "SetLinearVelocity", &RigidBody3d::SetLinearVelocity,
        "GetAngularVelocity", &RigidBody3d::GetAngularVelocity,
        "SetAngularVelocity", &RigidBody3d::SetAngularVelocity
    );

    sol::table inputModule = lua.create_named_table("Input");
    inputModule.set_function("IsKeyPressed", &Input::IsKeyJustPressed);
    inputModule.set_function("IsKeyHeld", &Input::IsKeyHeld);
    inputModule.set_function("IsMouseButtonPressed", &Input::IsMouseButtonJustPressed);
    inputModule.set_function("GetMouseDelta", &Input::GetMouseDelta);
    inputModule.set_function("SetMouseDeltaEnabled", &Input::SetMouseDeltaEnabled);
    inputModule.set_function("SetMouseCaptureEnabled", &Input::SetMouseCaptureEnabled);
    inputModule.set_function("IsMouseCaptureEnabled", &Input::IsMouseCaptureEnabled);

    sol::table keys = lua.create_table();
    keys["W"] = SDL_SCANCODE_W;
    keys["A"] = SDL_SCANCODE_A;
    keys["S"] = SDL_SCANCODE_S;
    keys["D"] = SDL_SCANCODE_D;
    keys["Q"] = SDL_SCANCODE_Q;
    keys["E"] = SDL_SCANCODE_E;
    keys["SPACE"] = SDL_SCANCODE_SPACE;
    keys["LCTRL"] = SDL_SCANCODE_LCTRL;
    keys["ESCAPE"] = SDL_SCANCODE_ESCAPE;
    keys["TAB"] = SDL_SCANCODE_TAB;

    inputModule["Key"] = keys;

    sol::table appModule = lua.create_named_table("App");
    appModule.set_function("Quit", [this] {
        RequestQuit();
    });
}

void ScriptManager::SetScript(Node3d *node, const std::string &path) const {
    ZoneScoped;
    ClearScript(node);

    if (!node || path.empty()) {
        return;
    }

    auto& lua = m_impl->lua;

    const std::string resolvedPath = ResourceManager::Get().ResolveAssetPath(path);
    if (resolvedPath.empty()) {
        std::cerr << "[Script Error] Failed to resolve script asset: " << path << std::endl;
        return;
    }

    sol::load_result loaded = lua.load_file(resolvedPath);
    if (!loaded.valid()) {
        const sol::error err = loaded;
        std::cerr << "[Script Error] Failed to load " << path << " (" << resolvedPath << "):\n" << err.what() << std::endl;
        return;
    }

    const sol::protected_function_result execResult = loaded();
    if (!execResult.valid()) {
        ReportProtectedFunctionError(execResult, "Failed to execute script file " + path + " (" + resolvedPath + ")");
        return;
    }

    const sol::object returned = execResult.get<sol::object>();
    if (!returned.is<sol::table>()) {
        std::cerr << "[Script Error] Script " << path << " must return a table." << std::endl;
        return;
    }

    sol::table scriptClass = returned.as<sol::table>();

    Impl::ScriptInstance instance;
    instance.table = lua.create_table();

    sol::table metatable = lua.create_table();
    metatable["__index"] = scriptClass;
    instance.table[sol::metatable_key] = metatable;

    instance.table["node"] = node;
    instance.table["script_path"] = resolvedPath;
    instance.table["script_name"] = path;

    instance.init = scriptClass["_init"];
    instance.ready = scriptClass["_ready"];
    instance.process = scriptClass["_process"];
    instance.physicsProcess = scriptClass["_physics_process"];

    m_impl->scripts[node] = std::move(instance);
}

void ScriptManager::ClearScript(Node3d *node) const {
    if (!node) {
        return;
    }

    m_impl->scripts.erase(node);
}

void ScriptManager::SetRuntimeEnabled(const bool enabled) const {
    m_impl->runtimeEnabled = enabled;
}

void ScriptManager::SetQuitHandler(std::function<void()> handler) const {
    m_impl->quitHandler = std::move(handler);
}

void ScriptManager::RequestQuit() const {
    if (!m_impl->quitHandler) {
        std::cerr << "[Script Warning] Quit requested, but no quit handler is registered.\n";
        return;
    }

    m_impl->quitHandler();
}

bool ScriptManager::IsRuntimeEnabled() const {
    return m_impl->runtimeEnabled;
}

void ScriptManager::CallReady(const Node3d *node) const {
    ZoneScoped;
    if (!m_impl->runtimeEnabled) {
        return;
    }

    const auto it = m_impl->scripts.find(const_cast<Node3d*>(node));
    if (it == m_impl->scripts.end()) {
        return;
    }

    if (!it->second.initialized && it->second.init.valid()) {
        const auto initResult = it->second.init(it->second.table);
        ReportProtectedFunctionError(initResult, "_init");
        if (!initResult.valid()) {
            return;
        }

        it->second.initialized = true;
    }

    if (!it->second.ready.valid()) {
        return;
    }

    const auto result = it->second.ready(it->second.table);
    ReportProtectedFunctionError(result, "_ready");
}

void ScriptManager::CallProcess(Node3d *node, float deltaTime) const {
    ZoneScoped;
    if (!m_impl->runtimeEnabled) {
        return;
    }

    const auto it = m_impl->scripts.find(node);
    if (it == m_impl->scripts.end() || !it->second.process.valid()) {
        return;
    }

    const auto result = it->second.process(it->second.table, deltaTime);
    ReportProtectedFunctionError(result, "_process");
}

void ScriptManager::CallPhysicsProcess(Node3d *node, float deltaTime) const {
    ZoneScoped;
    if (!m_impl->runtimeEnabled) {
        return;
    }

    const auto it = m_impl->scripts.find(node);
    if (it == m_impl->scripts.end() || !it->second.physicsProcess.valid()) {
        return;
    }

    const auto result = it->second.physicsProcess(it->second.table, deltaTime);
    ReportProtectedFunctionError(result, "_physics_process");
}
