
#ifndef MANGORENDERING_CORE_H
#define MANGORENDERING_CORE_H

#include <unordered_map>

#include "imgui.h"
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

    explicit Core() = default;
    ~Core() override;
    Core(const Core&) = delete;
    Core& operator=(const Core&) = delete;

    void Init();

    void Notification(Node3d* node, NodeNotification notification) override;

    [[nodiscard]] bool PollEvents() const; // returns false when window should close
    RenderStats RenderScene(const Node3d* sceneRoot, const CameraNode3d* camera, const Framebuffer* targetFbo) const;
    void SwapBuffers() const;
    void StepFrame(float deltaTime);
    void Process();

    void RegisterScene(Node3d* scene);
    static void UnregisterScene(Node3d* scene);
    void ChangeScene(std::unique_ptr<Node3d> scene);
    void RebuildNodeCache();

    void SetEditorCamera(CameraNode3d* camera);
    void SetGameCamera(CameraNode3d* camera);

    // this function should not be used for the game loop
    void SetActiveCamera(CameraNode3d* camera);
    void SetGlobalSkybox(std::unique_ptr<SkyboxNode3d> skybox);
    void SetCameraMode(CameraMode mode);

    static void BeginImGuiFrame();
    static void EndImGuiFrame();

    [[nodiscard]] Node3d* GetScene() const { return m_currentScene.get(); }
    [[nodiscard]] RenderApi& GetRenderer() const { return *m_renderer; }
    [[nodiscard]] Window* GetActiveWindow() const { return m_activeWindow.get(); }
    [[nodiscard]] CameraNode3d* GetActiveCamera() const { return m_activeCamera; }
    [[nodiscard]] CameraNode3d* GetEditorCamera() const { return m_editorCamera; }
    [[nodiscard]] SkyboxNode3d* GetGlobalSkybox() const { return m_globalSkybox.get(); }
    [[nodiscard]] CameraNode3d* GetGameCamera() const { return m_gameCamera; }

    [[nodiscard]] GLuint GetMainViewportTexture() const;
    [[nodiscard]] ImFont* GetMainFont() const { return m_mainFont; }
    [[nodiscard]] ImFont* GetMonoFont() const { return m_monoFont; }

    [[nodiscard]] static bool IsInScene(const Node3d* node, const Node3d* sceneRoot) ;

private:
    void InitRenderer();
    void InitImGui() const;

    void UncacheByRoot(Node3d* node);

    [[nodiscard]] bool IsNodeCached(const Node3d* node) const;
    [[nodiscard]] Node3d* CacheByRoot(Node3d* node);
    [[nodiscard]] Node3d* GetCachedRoot(const Node3d* node) const;

    void ApplyCameraMode();

    void RegisterLight(LightNode3d* light) const;
    void UnregisterLight(LightNode3d* light) const;

    float m_accumulator = 0.0f;
    static constexpr float FIXED_TIMESTEP = 1.0f / 60.0f;

    std::unique_ptr<RenderApi> m_renderer {};
    std::unique_ptr<Node3d> m_currentScene {};
    std::unique_ptr<SkyboxNode3d> m_globalSkybox {};
    std::unique_ptr<Window> m_activeWindow {};

    std::shared_ptr<Shader> m_defaultShader;

    ImFont* m_mainFont {};
    ImFont* m_monoFont {};

    std::vector<Node3d*> m_nodeCache;
    std::vector<RenderableNode3d*> m_renderableCache;
    std::vector<LightNode3d*> m_lightNodeCache;
    std::unordered_map<const Node3d*, Node3d*> m_nodeRootCache;
    std::unordered_map<const Node3d*, std::vector<Node3d*>> m_nodesByRoot;
    std::unordered_map<const Node3d*, std::vector<RenderableNode3d*>> m_renderablesByRoot;
    std::unordered_map<const Node3d*, std::vector<LightNode3d*>> m_lightsByRoot;

    std::unique_ptr<Framebuffer> m_mainFramebuffer;

    CameraMode m_cameraMode = CameraMode::Editor;
    CameraNode3d* m_editorCamera {};
    CameraNode3d* m_gameCamera {};
    CameraNode3d* m_activeCamera {};
};


#endif //MANGORENDERING_CORE_H
