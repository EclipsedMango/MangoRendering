
#ifndef MANGORENDERING_EDITOR_H
#define MANGORENDERING_EDITOR_H

#include "Core.h"
#include "Nodes/Node3d.h"
#include "imgui.h"
#include "ImGuizmo.h"

class Editor {
public:
    explicit Editor(Node3d* scene);
    ~Editor();

    Editor(const Editor&)            = delete;
    Editor& operator=(const Editor&) = delete;

    void Run();
    [[nodiscard]] Core& GetCore() { return m_core; }

    void SetGameCamera(CameraNode3d* camera) { m_gameCamera = camera; }

private:
    // panels
    void DrawMenuBar();
    void DrawSceneTree(Node3d* node);
    void DrawInspector() const;
    void DrawContentBrowser();
    void DrawGizmo();

    // helpers
    void OnPlay();
    void OnPause();
    void OnStop();

    void UpdateEditorCamera(float dt);

    // cameras
    std::unique_ptr<CameraNode3d> m_editorCamera;
    CameraNode3d* m_gameCamera = nullptr;

    // flycam controls
    bool  m_rmbLook = false;
    float m_moveSpeed = 5.0f;
    float m_mouseSensitivity = 0.08f;

    bool m_scrollToSelected = false;

    enum class State { Editing, Playing, Paused };

    Core m_core;
    State m_state = State::Editing;
    Node3d* m_selectedNode = nullptr;

    ImGuizmo::OPERATION m_gizmoOp = ImGuizmo::TRANSLATE;
    ImGuizmo::MODE m_gizmoMode = ImGuizmo::WORLD;
};


#endif //MANGORENDERING_EDITOR_H