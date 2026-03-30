
#ifndef MANGORENDERING_VIEWPORTWINDOW_H
#define MANGORENDERING_VIEWPORTWINDOW_H
#include <memory>
#include <string>

#include "imgui.h"
#include "Nodes/Node3d.h"

class Framebuffer;
class Editor;
class CameraNode3d;
class EditorCameraController;

class ViewportWindow {
public:
    ViewportWindow(Editor* editor, std::string  name);
    ~ViewportWindow();

    void LoadScene(std::unique_ptr<Node3d> scene);
    [[nodiscard]] std::unique_ptr<Node3d> DetachScene();

    void SaveSnapshot(std::unique_ptr<Node3d> scene);
    [[nodiscard]] std::unique_ptr<Node3d> TakeSnapshot();

    void Update(float deltaTime);

    void Draw();
    void DrawContent();

    [[nodiscard]] const std::string& GetName() const { return m_name; }
    [[nodiscard]] bool IsHovered() const { return m_viewportHovered; }
    [[nodiscard]] CameraNode3d* GetCamera() const { return m_camera.get(); }
    [[nodiscard]] EditorCameraController* GetCameraController() const { return m_cameraController.get(); }
    [[nodiscard]] ImVec2 GetViewportSize() const { return m_viewportSize; }
    [[nodiscard]] ImVec2 GetViewportPos() const { return m_viewportPos; }

    [[nodiscard]] Node3d* GetScene() const;

private:
    Editor* m_editor {};
    std::string m_name;

    ImVec2 m_viewportPos = {0, 0};
    ImVec2 m_viewportSize = {0, 0};
    bool m_viewportHovered = false;

    float m_timeSinceLastRender = 0.0f;

    std::unique_ptr<Node3d> m_scene;
    std::unique_ptr<Node3d> m_snapshot;
    std::unique_ptr<CameraNode3d> m_camera;
    std::unique_ptr<EditorCameraController> m_cameraController;
    std::unique_ptr<Framebuffer> m_framebuffer;
};


#endif //MANGORENDERING_VIEWPORTWINDOW_H