
#ifndef MANGORENDERING_CORE_H
#define MANGORENDERING_CORE_H

#include "RenderApi.h"
#include "Nodes/Node3d.h"
#include "Window.h"
#include "Nodes/RenderableNode3d.h"
#include "Nodes/Lights/LightNode3d.h"

class Core {
public:
    explicit Core(Node3d* scene);
    ~Core();

    Core(const Core&)            = delete;
    Core& operator=(const Core&) = delete;

    void Init();
    void Process();
    void ChangeScene(Node3d* scene);
    void RebuildNodeCache();

    void SetActiveCamera(Camera* camera);

    [[nodiscard]] RenderApi& GetRenderer() const { return *m_renderer; }

private:
    void InitRenderer();
    void InitImGui() const;

    static void BeginImGuiFrame();
    static void EndImGuiFrame();

    void BuildNodeCache(Node3d* node);

    void RegisterLight(LightNode3d* light) const;
    void UnregisterLight(LightNode3d* light) const;

    std::unique_ptr<RenderApi> m_renderer;
    Node3d* m_currentScene = nullptr;
    Window* m_activeWindow = nullptr;

    std::vector<Node3d*> m_nodeCache;
    std::vector<RenderableNode3d*> m_renderableCache;
    std::vector<LightNode3d*> m_lightCache;

    Camera* m_activeCamera = nullptr;
    bool m_mouseCaptured = true;
};


#endif //MANGORENDERING_CORE_H