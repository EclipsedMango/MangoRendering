#ifndef MANGORENDERING_RENDERAPI_H
#define MANGORENDERING_RENDERAPI_H

#include <SDL3/SDL.h>
#include <vector>
#include <memory>

#include "Renderer/Pipeline/ClusterSystem.h"
#include "Renderer/Pipeline/LightManager.h"
#include "Renderer/Pipeline/ShadowRenderer.h"
#include "Window.h"
#include "Nodes/CameraNode3d.h"
#include "Nodes/MeshNode3d.h"
#include "Nodes/SkyboxNode3d.h"
#include "Renderer/Lights/DirectionalLight.h"
#include "Renderer/Meshes/Mesh.h"
#include "Renderer/Shader.h"
#include "Renderer/Buffers/UniformBuffer.h"
#include "Renderer/Buffers/ShaderStorageBuffer.h"
#include "Renderer/Lights/PointLight.h"
#include "Renderer/Lights/SpotLight.h"
#include "Renderer/Shadows/CascadedShadowMap.h"

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

    static void ClearColour(const glm::vec4& colour);
    void HandleResizeEvent(const SDL_Event& event) const;

    void SetActiveCamera(CameraNode3d* camera);
    void UploadCameraData() const;

    void AddDirectionalLight(DirectionalLight* light) const;
    void RemoveDirectionalLight(DirectionalLight* light) const;

    void AddPointLight(PointLight* light) const;
    void RemovePointLight(PointLight* light) const;

    void AddSpotLight(SpotLight* light) const;
    void RemoveSpotLight(SpotLight* light) const;

    void SubmitMesh(MeshNode3d* node) { m_meshQueue.push_back(node); }
    void SetSkybox(SkyboxNode3d* skybox)    { m_skybox = skybox; }

    [[nodiscard]] ShaderStorageBuffer* GetLightGridSsbo()   const { return m_clusterSystem->GetLightGridSsbo(); }
    [[nodiscard]] ShaderStorageBuffer* GetGlobalCountSsbo() const { return m_clusterSystem->GetGlobalCountSsbo(); }

    [[nodiscard]] const std::vector<CascadedShadowMap*>& GetCascadedShadowMaps() const { return m_shadowRenderer->GetCascadedShadowMaps(); }
    [[nodiscard]] uint32_t GetShadowedPointLightCount()  const { return m_shadowRenderer->GetShadowedPointLightCount(); }
    [[nodiscard]] uint32_t GetMaxShadowedPointLights()   const { return ShadowRenderer::MAX_SHADOWED_POINT_LIGHTS; }
    [[nodiscard]] const std::vector<ShadowedPointLightDebug>& GetShadowedPointLightsDebug() const { return m_shadowRenderer->GetShadowedPointLightsDebug(); }

    void Flush();

    static void DrawMesh(const Mesh& mesh, const Shader& shader);
    void DrawClusterVisualizer();

    // Debug
    void SetDebugMode(int mode);
    void SetDebugCascade(int cascade);

    [[nodiscard]] uint32_t GetPointLightCount() const { return m_lightManager->GetPointLightCount(); }
    [[nodiscard]] RenderStats GetStats() const { return m_stats; }
    [[nodiscard]] int GetDebugMode() const { return m_debugMode; }
    [[nodiscard]] int GetDebugCascade() const { return m_debugCascade; }

private:
    void InitGLResources(); // called once after GLAD is loaded
    void InitDepthPass();

    void DrawMeshNode(const MeshNode3d* node);
    void DrawMeshNodeDepth(const MeshNode3d* node) const;

    static void BeginZPrepass();
    static void EndZPrepass();

    void RebuildClusters() const;
    void RunLightCulling() const;
    void RenderMainPass();
    void RenderTransparentPass();

    // constants
    static constexpr uint32_t CLUSTER_DIM_X = 16;
    static constexpr uint32_t CLUSTER_DIM_Y = 9;
    static constexpr uint32_t CLUSTER_DIM_Z = 24;
    static constexpr uint32_t NUM_CLUSTERS = CLUSTER_DIM_X * CLUSTER_DIM_Y * CLUSTER_DIM_Z;
    static constexpr uint32_t MAX_LIGHTS_PER_CLUSTER = 100;
    static constexpr uint32_t MAX_DIR_LIGHTS = 4;
    static constexpr uint32_t MAX_TEXTURE_SLOTS = 16;

    std::vector<std::unique_ptr<Window>> m_windows;

    CameraNode3d* m_activeCamera = nullptr;
    SkyboxNode3d* m_skybox = nullptr;

    std::unique_ptr<ClusterSystem> m_clusterSystem;
    std::unique_ptr<LightManager> m_lightManager;
    std::unique_ptr<ShadowRenderer> m_shadowRenderer;

    std::unique_ptr<UniformBuffer> m_cameraUbo;
    std::unique_ptr<Shader> m_depthShader;

    std::vector<MeshNode3d*> m_meshQueue;
    std::vector<MeshNode3d*> m_transparentQueue;

    std::unique_ptr<Mesh> m_debugClusterMesh;
    std::unique_ptr<Shader> m_debugClusterShader;

    RenderStats m_stats;
    int m_debugMode = 0;
    int m_debugCascade = 0;
};

#endif //MANGORENDERING_RENDERAPI_H