#ifndef MANGORENDERING_RENDERAPI_H
#define MANGORENDERING_RENDERAPI_H

#include <SDL3/SDL.h>
#include <vector>
#include <memory>
#include <string>

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
#include "Renderer/Pipeline/IBLPrecomputer.h"
#include "Renderer/Shadows/CascadedShadowMap.h"

class PortalNode3d;
class Frustum;

struct RenderStats {
    uint32_t drawCalls = 0;
    uint32_t shadowDrawCalls = 0;
    uint32_t culled = 0;
    uint32_t triangles = 0;
    uint32_t submitted = 0;
    float animatorUpdateMs = 0.0f;
    float skinUploadMs = 0.0f;
    float drawSubmitMs = 0.0f;
};

class RenderApi {
public:
    enum class ToneMapOperator : int {
        Linear = 0,
        Reinhard = 1,
        Filmic = 2,
        Aces = 3,
        Stylized = 4,
    };

    struct XeGtaoSettings {
        bool enabled = true;
        bool denoise = true;
        int quality = 2;
        float intensity = 1.0f;
        float radius = 1.0f;
        float falloffRange = 0.45f;
        float finalPower = 1.5f;
        float thinOccluderCompensation = 0.0f;
        float depthMipSamplingOffset = 3.15f;
    };

    // initializes SDL and sets GL attributes, MUST be called before constructing RenderApi
    static void InitSDL();

    RenderApi() = default;
    ~RenderApi();

    RenderApi(const RenderApi&) = delete;
    RenderApi& operator=(const RenderApi&) = delete;
    RenderApi(RenderApi&&) = delete;
    RenderApi& operator=(RenderApi&&) = delete;

    std::unique_ptr<Window> CreateWindow(const char* title, glm::vec2 size, Uint32 flags);
    static void ClearColour(const glm::vec4& colour);
    void HandleResizeEvent(const SDL_Event& event) const;

    void AddDirectionalLight(DirectionalLight* light) const;
    void RemoveDirectionalLight(DirectionalLight* light) const;
    void AddPointLight(PointLight* light) const;
    void RemovePointLight(PointLight* light) const;
    void AddSpotLight(SpotLight* light) const;
    void RemoveSpotLight(SpotLight* light) const;

    void SubmitRenderable(RenderableNode3d* node);
    void SubmitPortal(PortalNode3d* portal);
    void ClearQueues();
    void SubmitMesh(MeshNode3d* node) { m_meshQueue.push_back(node); }
    void SetSkybox(SkyboxNode3d* skybox);
    void UploadCameraData(const CameraNode3d* camera) const;

    [[nodiscard]] ShaderStorageBuffer* GetLightGridSsbo() const { return m_clusterSystem->GetLightGridSsbo(); }
    [[nodiscard]] ShaderStorageBuffer* GetGlobalCountSsbo() const { return m_clusterSystem->GetGlobalCountSsbo(); }
    [[nodiscard]] const std::vector<CascadedShadowMap*>& GetCascadedShadowMaps() const { return m_shadowRenderer->GetCascadedShadowMaps(); }
    [[nodiscard]] uint32_t GetShadowedPointLightCount() const { return m_shadowRenderer->GetShadowedPointLightCount(); }
    [[nodiscard]] static uint32_t GetMaxShadowedPointLights() { return ShadowRenderer::MAX_SHADOWED_POINT_LIGHTS; }
    [[nodiscard]] const std::vector<ShadowedPointLightDebug>& GetShadowedPointLightsDebug() const { return m_shadowRenderer->GetShadowedPointLightsDebug(); }

    static void ApplyMaterialCull(const Material* mat);
    RenderStats RenderScene(const CameraNode3d* camera, const Framebuffer* targetFbo, bool clearFbo);
    RenderStats RenderSceneWithPortals(const CameraNode3d* camera, const Framebuffer* targetFbo, int maxPortalDepth);
    void FinalizePostProcess(const Framebuffer* targetFbo) const;
    void SetExposure(float exposure);
    void SetToneMapOperator(ToneMapOperator op);
    void DrawGrid(const CameraNode3d* camera, const Framebuffer* targetFbo) const;
    static void DrawMesh(const Mesh& mesh, const Shader& shader);

    // Debug
    void SetDebugMode(int mode);
    void SetDebugCascade(int cascade);

    [[nodiscard]] uint32_t GetPointLightCount() const { return m_lightManager->GetPointLightCount(); }
    [[nodiscard]] int GetDebugMode() const { return m_debugMode; }
    [[nodiscard]] int GetDebugCascade() const { return m_debugCascade; }
    [[nodiscard]] XeGtaoSettings& GetXeGtaoSettings() { return m_xeGtaoSettings; }
    [[nodiscard]] const XeGtaoSettings& GetXeGtaoSettings() const { return m_xeGtaoSettings; }

private:
    void InitGLResources(); // called once after GLAD is loaded
    void InitDepthPass();

    // for frustum culling
    [[nodiscard]] static bool IsCulled(const MeshNode3d* node, const Frustum& frustum, RenderStats& stats);
    static void SubmitToGpu(const MeshNode3d* node, const Shader* shader, RenderStats& stats);
    void DrawMeshNodeDepth(const MeshNode3d* node, RenderStats& stats) const;

    static void BeginZPrepass();
    static void EndZPrepass();

    void RebuildClusters(const CameraNode3d* camera, const Framebuffer* targetFbo) const;
    void RunLightCulling() const;
    void EnsureXeGtaoResources(const Framebuffer* targetFbo);
    void DestroyXeGtaoResources();
    void RunXeGtao(const CameraNode3d* camera, const Framebuffer* sourceFbo);
    void EnsurePostProcessResources(const Framebuffer* targetFbo);
    void RunPostProcess(const Framebuffer* targetFbo) const;
    [[nodiscard]] GLuint GetXeGtaoOutputTexture() const;

    RenderStats RenderView(const CameraNode3d* camera, const Framebuffer* targetFbo, bool clearFbo, const PortalNode3d* excludedPortal = nullptr, bool isMainPass = true);

    void RenderMainPass(const CameraNode3d* camera, const Framebuffer* targetFbo, const std::vector<MeshNode3d*>& opaqueQueue, RenderStats& stats) const;
    static void RenderTransparentPass(const Frustum& frustum, const std::vector<MeshNode3d*>& transparentQueue, RenderStats& stats);

    void RenderPortalPasses(const CameraNode3d* camera, const Framebuffer* targetFbo, int remainingDepth, int currentStencil = 0);
    static glm::mat4 ComputePortalView(const CameraNode3d* mainCamera, const PortalNode3d* sourcePortal, const PortalNode3d* destPortal);
    static glm::mat4 ObliqueProjection(const glm::mat4& projMat, const glm::mat4& virtualView, const PortalNode3d* destPortal);

    void DrawPortalMask(const PortalNode3d* portal, int currentStencil, const CameraNode3d* camera) const;
    void RestorePortalMask(const PortalNode3d* portal, int nextStencil, const CameraNode3d* camera) const;

    // constants
    static constexpr uint32_t CLUSTER_DIM_X = 16;
    static constexpr uint32_t CLUSTER_DIM_Y = 9;
    static constexpr uint32_t CLUSTER_DIM_Z = 24;
    static constexpr uint32_t NUM_CLUSTERS = CLUSTER_DIM_X * CLUSTER_DIM_Y * CLUSTER_DIM_Z;

    std::vector<Window*> m_windows;
    SkyboxNode3d* m_skybox = nullptr;

    std::unique_ptr<ClusterSystem> m_clusterSystem;
    std::unique_ptr<LightManager> m_lightManager;
    std::unique_ptr<ShadowRenderer> m_shadowRenderer;

    std::unique_ptr<UniformBuffer> m_cameraUbo;
    std::shared_ptr<Shader> m_depthShader;
    std::shared_ptr<Shader> m_gridShader;
    std::shared_ptr<Shader> m_postProcessShader;
    std::shared_ptr<Shader> m_xeGtaoPrefilterShader;
    std::shared_ptr<Shader> m_xeGtaoMainShader;
    std::shared_ptr<Shader> m_xeGtaoDenoiseShader;
    GLuint m_gridVao = 0;
    GLuint m_postProcessVao = 0;
    std::unique_ptr<Framebuffer> m_postProcessFramebuffer;
    XeGtaoSettings m_xeGtaoSettings;
    GLuint m_xeGtaoViewDepthTexture = 0;
    GLuint m_xeGtaoRawAoTexture = 0;
    GLuint m_xeGtaoFilteredAoTexture = 0;
    uint32_t m_xeGtaoWidth = 0;
    uint32_t m_xeGtaoHeight = 0;
    uint32_t m_xeGtaoMipCount = 0;
    bool m_hasValidXeGtao = false;
    float m_exposure = 1.0f;
    ToneMapOperator m_toneMapOperator = ToneMapOperator::Aces;

    std::vector<MeshNode3d*> m_meshQueue;
    std::vector<PortalNode3d*> m_portalQueue;

    IBLPrecomputer::Result m_ibl;
    bool m_hasIbl = false;

    int m_debugMode = 0;
    int m_debugCascade = 0;
};

#endif //MANGORENDERING_RENDERAPI_H
