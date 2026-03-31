
#ifndef MANGORENDERING_EDITOR_H
#define MANGORENDERING_EDITOR_H

#include "GizmoSystem.h"
#include "../Core.h"
#include "UI/InspectorPanel.h"
#include "UI/SceneTreePanel.h"
#include "UI/ViewportWindow.h"

class ViewportWindow;
struct SceneTab {
    std::string name;
    bool open = true;
};

class Editor {
public:
    explicit Editor(std::unique_ptr<Node3d> scene);
    ~Editor();

    Editor(const Editor&) = delete;
    Editor& operator=(const Editor&) = delete;

    enum class State { Editing, Playing, Paused };

    void Run();

    [[nodiscard]] Core& GetCore() { return m_core; }
    [[nodiscard]] ViewportWindow* GetActiveViewport() const { return m_activeViewport; }
    [[nodiscard]] SceneTreePanel& GetSceneTree() { return m_sceneTree; }
    [[nodiscard]] GizmoSystem& GetGizmoSystem() { return m_gizmoSystem; }
    [[nodiscard]] State GetState() const { return m_state; }

    void SetGameCamera(std::unique_ptr<CameraNode3d> camera) { m_gameCamera = std::move(camera); }

private:
    // panels
    void DrawViewportTabs();
    void DrawMenuBar();
    void DrawContentBrowser();
    void DrawCameraSpeedIndication(float alpha) const;

    // helpers
    void OnPlay();
    void OnPause();
    void OnStop();

    static CameraNode3d* FindGameCamera(Node3d* node);

    [[nodiscard]] bool IsAnyViewportLooking() const;
    std::vector<std::unique_ptr<ViewportWindow>> m_viewports;
    std::unique_ptr<ViewportWindow> m_mainViewport;
    ViewportWindow* m_activeViewport = nullptr;

    // cameras
    std::unique_ptr<CameraNode3d> m_gameCamera {};

    Core m_core;
    InspectorPanel m_inspector;
    SceneTreePanel m_sceneTree;
    GizmoSystem m_gizmoSystem;

    float m_speedIndicatorTimer = 0.0f;
    constexpr static float kSpeedIndicatorFadeTime = 1.2f;

    State m_state = State::Editing;

    std::vector<SceneTab> m_sceneTabs;
    int m_activeTab = 0;

    float m_cpuTime = 0.0f;
};

#endif //MANGORENDERING_EDITOR_H