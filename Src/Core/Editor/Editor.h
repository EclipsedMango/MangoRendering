
#ifndef MANGORENDERING_EDITOR_H
#define MANGORENDERING_EDITOR_H

#include "../Core.h"
#include "Nodes/Node3d.h"
#include "imgui.h"
#include "ImGuizmo.h"
#include "InspectorPanel.h"
#include "SceneTreePanel.h"

class Editor {
public:
    explicit Editor(Node3d* scene);
    ~Editor();

    Editor(const Editor&) = delete;
    Editor& operator=(const Editor&) = delete;

    void Run();
    [[nodiscard]] Core& GetCore() { return m_core; }

    void SetGameCamera(CameraNode3d* camera) { m_gameCamera = camera; }

private:
    // panels
    void DrawViewport();
    void DrawMenuBar();
    void DrawContentBrowser();
    void DrawGizmo();

    // helpers
    void OnPlay();
    void OnPause();
    void OnStop();

    void UpdateEditorCamera(float dt);
    void DrawWorldOBB(const glm::mat4& worldMatrix, const glm::vec3& localCenter, const glm::vec3& half, ImU32 color) const;

    static void ExpandLocalAABB(Node3d* root, const Node3d* node, glm::mat4& rootWorldInv, glm::vec3& minB, glm::vec3& maxB);
    void DrawWorldDirections();
    bool WorldToScreen(const glm::vec3& worldPos, const glm::mat4& viewProj, ImVec2& out) const;

    ImVec2 m_viewportPos = {0, 0};
    ImVec2 m_viewportSize = {0, 0};

    // cameras
    std::unique_ptr<CameraNode3d> m_editorCamera;
    CameraNode3d* m_gameCamera = nullptr;

    bool m_wasUsingGizmo = false;
    std::vector<glm::vec3> m_gizmoInitialScales;

    // flycam controls
    bool m_rmbLook = false;
    bool m_viewportHovered = false;
    bool m_snapObjectMovement = false;
    float m_moveSpeed = 5.0f;
    float m_mouseSensitivity = 0.08f;

    enum class State { Editing, Playing, Paused };

    Core m_core;
    InspectorPanel m_inspector;
    SceneTreePanel m_sceneTree;
    State m_state = State::Editing;

    ImGuizmo::OPERATION m_gizmoOp = ImGuizmo::TRANSLATE;
    ImGuizmo::MODE m_gizmoMode = ImGuizmo::WORLD;

    float m_cpuTime = 0.0f;
};


#endif //MANGORENDERING_EDITOR_H