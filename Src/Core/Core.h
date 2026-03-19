
#ifndef MANGORENDERING_CORE_H
#define MANGORENDERING_CORE_H

#include "RenderApi.h"
#include "TreeListener.h"
#include "Nodes/Node3d.h"
#include "Window.h"
#include "Nodes/CameraNode3d.h"
#include "Nodes/RenderableNode3d.h"
#include "Nodes/SkyboxNode3d.h"
#include "Nodes/Lights/LightNode3d.h"

class Core : public TreeListener {
public:
    enum class CameraMode { Editor, Game };

    explicit Core(Node3d* scene);
    ~Core() override;

    Core(const Core&)            = delete;
    Core& operator=(const Core&) = delete;

    void Init();

    void Notification(Node3d* node, NodeNotification notification) override;

    bool PollEvents() const; // returns false when window should close
    void RenderScene() const;
    void SwapBuffers() const;
    void StepFrame(float deltaTime);
    void Process();

    void ChangeScene(Node3d* scene);
    void RebuildNodeCache();

    void SetEditorCamera(CameraNode3d* camera);
    void SetGameCamera(CameraNode3d* camera);

    // this function should not be used for the game loop
    void SetActiveCamera(CameraNode3d* camera);

    void SetCameraMode(CameraMode mode);

    static void BeginImGuiFrame();
    static void EndImGuiFrame();

    [[nodiscard]] Node3d* GetScene() const { return m_currentScene; }
    [[nodiscard]] RenderApi& GetRenderer() const { return *m_renderer; }
    [[nodiscard]] Window* GetActiveWindow() const { return m_activeWindow; }
    [[nodiscard]] CameraNode3d* GetActiveCamera() const { return m_activeCamera; }
    [[nodiscard]] CameraNode3d* GetEditorCamera() const { return m_editorCamera; }
    [[nodiscard]] CameraNode3d* GetGameCamera() const { return m_gameCamera; }
    [[nodiscard]] Shader* GetDefaultShader() const { return m_defaultShader.get(); }

private:
    void InitRenderer();
    void InitImGui() const;

    void BuildNodeCache(Node3d* node);
    [[nodiscard]] bool IsNodeCached(const Node3d* node) const;

    void ApplyCameraMode();

    void RegisterLight(LightNode3d* light) const;
    void UnregisterLight(LightNode3d* light) const;

    float m_accumulator = 0.0f;
    static constexpr float FIXED_TIMESTEP = 1.0f / 60.0f;

    std::unique_ptr<RenderApi> m_renderer;
    Node3d* m_currentScene = nullptr;
    Window* m_activeWindow = nullptr;

    std::vector<Node3d*> m_nodeCache;
    std::vector<RenderableNode3d*> m_renderableCache;
    std::vector<LightNode3d*> m_lightNodeCache;
    SkyboxNode3d* m_activeSkybox = nullptr;

    CameraMode m_cameraMode = CameraMode::Editor;
    CameraNode3d* m_editorCamera = nullptr;
    CameraNode3d* m_gameCamera   = nullptr;
    CameraNode3d* m_activeCamera = nullptr;
    std::unique_ptr<Shader> m_defaultShader;
};


#endif //MANGORENDERING_CORE_H