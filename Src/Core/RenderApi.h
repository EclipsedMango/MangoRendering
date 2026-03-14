#ifndef MANGORENDERING_RENDERAPI_H
#define MANGORENDERING_RENDERAPI_H

#include <SDL3/SDL.h>
#include <vector>
#include <memory>

#include "Renderer/Pipeline/ClusterSystem.h"
#include "Renderer/Pipeline/LightManager.h"
#include "Renderer/Pipeline/ShadowRenderer.h"
#include "Window.h"
#include "Scene/Camera.h"
#include "Nodes/Lights/DirectionalLight.h"
#include "Renderer/Mesh.h"
#include "Scene/Object.h"
#include "Renderer/Shader.h"
#include "Renderer/Buffers/UniformBuffer.h"
#include "Renderer/Buffers/ShaderStorageBuffer.h"
#include "Nodes/Lights/PointLight.h"
#include "Nodes/Lights/SpotLight.h"
#include "Renderer/CascadedShadowMap.h"
#include "Renderer/Skybox.h"

struct RenderStats {
    uint32_t drawCalls       = 0;
    uint32_t shadowDrawCalls = 0;
    uint32_t culled          = 0;
    uint32_t triangles       = 0;
    uint32_t submitted       = 0;
};

class RenderApi {
public:
    // Initializes SDL and sets GL attributes, MUST be called before constructing RenderApi
    static void InitSDL();

    RenderApi() = default;
    ~RenderApi();

    RenderApi(const RenderApi&)            = delete;
    RenderApi& operator=(const RenderApi&) = delete;
    RenderApi(RenderApi&&)                 = delete;
    RenderApi& operator=(RenderApi&&)      = delete;

    Window* CreateWindow(const char* title, glm::vec2 size, Uint32 flags);
    void ClearColour(const glm::vec4& colour);
    void HandleResizeEvent(const SDL_Event& event);

    void SetActiveCamera(Camera* camera);
    void UploadCameraData();

    void AddDirectionalLight(DirectionalLight* light);
    void RemoveDirectionalLight(DirectionalLight* light);

    void AddPointLight(PointLight* light);
    void RemovePointLight(PointLight* light);

    void AddSpotLight(SpotLight* light);
    void RemoveSpotLight(SpotLight* light);

    [[nodiscard]] ShaderStorageBuffer* GetLightGridSsbo()   const { return m_clusterSystem->GetLightGridSsbo(); }
    [[nodiscard]] ShaderStorageBuffer* GetGlobalCountSsbo() const { return m_clusterSystem->GetGlobalCountSsbo(); }

    [[nodiscard]] const std::vector<CascadedShadowMap*>& GetCascadedShadowMaps() const { return m_shadowRenderer->GetCascadedShadowMaps(); }
    [[nodiscard]] uint32_t GetShadowedPointLightCount()  const { return m_shadowRenderer->GetShadowedPointLightCount(); }
    [[nodiscard]] uint32_t GetMaxShadowedPointLights()   const { return ShadowRenderer::MAX_SHADOWED_POINT_LIGHTS; }
    [[nodiscard]] const std::vector<ShadowedPointLightDebug>& GetShadowedPointLightsDebug() const { return m_shadowRenderer->GetShadowedPointLightsDebug(); }

    void Submit(const Object* object);
    void Flush();

    void DrawMesh(const Mesh& mesh, const Shader& shader);
    void DrawClusterVisualizer();

    void SetSkybox(Skybox* skybox);

    // Debug
    void SetDebugMode(int mode);
    void SetDebugCascade(int cascade);

    [[nodiscard]] uint32_t GetPointLightCount() const { return m_lightManager->GetPointLightCount(); }
    [[nodiscard]] RenderStats GetStats()        const { return m_stats; }
    [[nodiscard]] int GetDebugMode()            const { return m_debugMode; }
    [[nodiscard]] int GetDebugCascade()         const { return m_debugCascade; }

private:
    void InitGLResources(); // called once after GLAD is loaded
    void InitDepthPass();

    void DrawObject(const Object* object);
    void DrawObjectDepth(const Object* object);
    void BeginZPrepass();
    void EndZPrepass();

    void RebuildClusters();
    void RunLightCulling();
    void RenderMainPass();

    // constants
    static constexpr uint32_t CLUSTER_DIM_X         = 16;
    static constexpr uint32_t CLUSTER_DIM_Y         = 9;
    static constexpr uint32_t CLUSTER_DIM_Z         = 24;
    static constexpr uint32_t NUM_CLUSTERS           = CLUSTER_DIM_X * CLUSTER_DIM_Y * CLUSTER_DIM_Z;
    static constexpr uint32_t MAX_LIGHTS_PER_CLUSTER = 100;
    static constexpr uint32_t MAX_DIR_LIGHTS         = 4;
    static constexpr uint32_t MAX_TEXTURE_SLOTS      = 16;

    std::vector<std::unique_ptr<Window>> m_windows;

    Camera* m_activeCamera = nullptr;
    Skybox* m_skybox       = nullptr;

    std::unique_ptr<ClusterSystem> m_clusterSystem;
    std::unique_ptr<LightManager> m_lightManager;
    std::unique_ptr<ShadowRenderer> m_shadowRenderer;

    std::unique_ptr<UniformBuffer> m_cameraUbo;

    std::unique_ptr<Shader> m_depthShader;

    std::vector<const Object*> m_renderQueue;

    std::unique_ptr<Mesh>   m_debugClusterMesh;
    std::unique_ptr<Shader> m_debugClusterShader;

    RenderStats m_stats;
    int m_debugMode    = 0;
    int m_debugCascade = 0;
};

#endif //MANGORENDERING_RENDERAPI_H