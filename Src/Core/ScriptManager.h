
#ifndef MANGORENDERING_SCRIPTMANAGER_H
#define MANGORENDERING_SCRIPTMANAGER_H

#include <memory>
#include <string>

class Node3d;

class ScriptManager {
public:
    static ScriptManager& Get() {
        static ScriptManager instance;
        return instance;
    }

    void Init();

    void SetScript(Node3d* node, const std::string& path) const;
    void ClearScript(Node3d* node) const;

    void CallReady(Node3d* node) const;
    void CallProcess(Node3d* node, float deltaTime) const;
    void CallPhysicsProcess(Node3d* node, float deltaTime) const;

    ScriptManager(const ScriptManager&) = delete;
    ScriptManager& operator=(const ScriptManager&) = delete;

private:
    ScriptManager();
    ~ScriptManager();

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};


#endif //MANGORENDERING_SCRIPTMANAGER_H