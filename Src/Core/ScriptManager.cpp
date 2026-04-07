
#include "ScriptManager.h"

#include "LuaIncludes.hpp"
#include <sol/sol.hpp>
#include <iostream>

#include "Input.h"
#include "ResourceManager.h"
#include "Nodes/Node3d.h"

struct ScriptManager::Impl {
    struct ScriptInstance {
        sol::table table;
        sol::protected_function ready;
        sol::protected_function process;
        sol::protected_function physicsProcess;
    };

    sol::state lua;
    std::unordered_map<Node3d*, ScriptInstance> scripts;
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

void ScriptManager::Init() {
    auto& lua = m_impl->lua;

    lua.open_libraries(
        sol::lib::base,
        sol::lib::package,
        sol::lib::math,
        sol::lib::string,
        sol::lib::table
    );

    lua.set_panic([](lua_State* L) -> int {
        const char* msg = lua_tostring(L, -1);
        std::cerr << "[Lua Panic Error] " << (msg ? msg : "Unknown Error") << std::endl;
        return 0;
    });

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

    lua.new_usertype<Node3d>("Node3d",
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
        "GetParent", &Node3d::GetParent
    );

    lua.new_usertype<Input>("Input",
        "IsKeyPressed", &Input::IsKeyJustPressed,
        "IsMouseButtonPressed", &Input::IsMouseButtonJustPressed,
        "GetMouseDelta", &Input::GetMouseDelta
    );
}

void ScriptManager::SetScript(Node3d *node, const std::string &path) const {
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

    instance.ready = scriptClass["_ready"];
    instance.process = scriptClass["_process"];
    instance.physicsProcess = scriptClass["_physics_process"];

    const sol::protected_function initFunc = scriptClass["_init"];
    if (initFunc.valid()) {
        const sol::protected_function_result initResult = initFunc(instance.table);
        if (!initResult.valid()) {
            ReportProtectedFunctionError(initResult, "Script _init for " + path);
            return;
        }
    }

    m_impl->scripts[node] = std::move(instance);
}

void ScriptManager::ClearScript(Node3d *node) const {
    if (!node) {
        return;
    }

    m_impl->scripts.erase(node);
}

void ScriptManager::CallReady(Node3d *node) const {
    const auto it = m_impl->scripts.find(node);
    if (it == m_impl->scripts.end() || !it->second.ready.valid()) {
        return;
    }

    const auto result = it->second.ready(it->second.table);
    ReportProtectedFunctionError(result, "_ready");
}

void ScriptManager::CallProcess(Node3d *node, float deltaTime) const {
    const auto it = m_impl->scripts.find(node);
    if (it == m_impl->scripts.end() || !it->second.process.valid()) {
        return;
    }

    const auto result = it->second.process(it->second.table, deltaTime);
    ReportProtectedFunctionError(result, "_process");
}

void ScriptManager::CallPhysicsProcess(Node3d *node, float deltaTime) const {
    const auto it = m_impl->scripts.find(node);
    if (it == m_impl->scripts.end() || !it->second.physicsProcess.valid()) {
        return;
    }

    const auto result = it->second.physicsProcess(it->second.table, deltaTime);
    ReportProtectedFunctionError(result, "_physics_process");
}
