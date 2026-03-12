#ifndef MANGORENDERING_RENDERAPI_H
#define MANGORENDERING_RENDERAPI_H

#include <SDL3/SDL.h>
#include <vector>

#include "Window.h"
#include "Scene/Camera.h"
#include "Lights/DirectionalLight.h"
#include "Renderer/VertexArray.h"
#include "Renderer/Mesh.h"
#include "Scene/Object.h"
#include "Renderer/Shader.h"
#include "Buffers/UniformBuffer.h"
#include "Buffers/ShaderStorageBuffer.h"
#include "Lights/DirectionalLight.h"
#include "Lights/PointLight.h"
#include "Lights/SpotLight.h"
#include "Renderer/CascadedShadowMap.h"
#include "Renderer/PointLightShadowMap.h"
#include "Renderer/Skybox.h"

struct ShadowedPointLightDebug {
    uint32_t lightIndex = 0;
    uint32_t slot = 0;
    float score = 0.0f;
    float radius = 0.0f;
    float farPlane = 0.0f;
    glm::vec3 position{0.0f};
    float distanceToCamera = 0.0f;
};

class RenderApi {
public:
    static void Init();
    static Window* CreateWindow(const char* title, glm::vec2 size, Uint32 flags);
    static void ClearColour(const glm::vec4& colour);
    static void HandleResizeEvent(const SDL_Event& event);

    static void SetActiveCamera(Camera* camera);
    static void UploadCameraData();

    static void AddDirectionalLight(DirectionalLight* light);
    static void RemoveDirectionalLight(DirectionalLight* light);

    static void AddPointLight(PointLight* light);
    static void RemovePointLight(PointLight* light);

    static void AddSpotLight(SpotLight* light);
    static void RemoveSpotLight(SpotLight* light);

    static ShaderStorageBuffer* GetLightGridSsbo() { return m_lightGridSsbo; }
    static ShaderStorageBuffer* GetGlobalCountSsbo() { return m_globalCountSsbo; }
    static const std::vector<CascadedShadowMap*>& GetCascadedShadowMaps() { return m_cascadedShadowMaps; }

    static void Submit(const Object* object);
    static void Flush();

    static void InitDepthPass();
    static void DrawObjectDepth(const Object* object);

    static void BeginZPrepass();
    static void EndZPrepass();

    static void UploadLightData();
    static void RebuildClusters();
    static void RunLightCulling();
    static float CalculateLightRadius(const glm::vec3& color, float intensity, float constant, float linear, float quadratic);

    static void RenderDirectionalShadows();

    static void EnsurePointShadowMetaBuffer(size_t pointLightCount);
    static float ScorePointLight(const PointLight* l, const Camera* cam);
    static void BuildPointShadowFaceMatrices(const glm::vec3& lightPos, float nearPlane, float farPlane, glm::mat4 outVP[6]);
    static void RenderPointLightShadows();

    static VertexArray* CreateBuffer(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

    static void DrawMesh(const Mesh& mesh, const Shader& shader);
    static void DrawObject(const Object* object);
    static void DrawClusterVisualizer();

    static void SetSkybox(Skybox* skybox);

    // debug stuff
    static void SetDebugMode(int mode);
    static void SetDebugCascade(int cascade);
    static uint32_t GetPointLightCount() { return static_cast<uint32_t>(m_pointLights.size()); }
    static uint32_t GetShadowedPointLightCount() { return static_cast<uint32_t>(m_shadowedPointLightsDebug.size()); }
    static uint32_t GetMaxShadowedPointLights() { return MAX_SHADOWED_POINT_LIGHTS; }
    static const std::vector<ShadowedPointLightDebug>& GetShadowedPointLightsDebug() { return m_shadowedPointLightsDebug; }

    [[nodiscard]] static uint32_t GetDrawCallCount() { return m_drawCallCount; }
    [[nodiscard]] static uint32_t GetShadowDrawCallCount() { return m_shadowDrawCallCount; }
    [[nodiscard]] static uint32_t GetCulledCount() { return m_culledCount; }
    [[nodiscard]] static uint32_t GetTriangleCount() { return m_triangleCount; }
    [[nodiscard]] static uint32_t GetSubmittedCount() { return m_submittedCount; }
    [[nodiscard]] static int GetDebugMode() { return m_debugMode; }
    [[nodiscard]] static int GetDebugCascade() { return m_debugCascade; }

private:
    static bool m_gladInitialized;
    static Camera* m_activeCamera;
    static UniformBuffer* m_cameraUbo;
    static std::vector<Window*> m_windows;
    static Skybox* m_skybox;

    static std::vector<DirectionalLight*> m_directionalLights;
    static std::vector<CascadedShadowMap*> m_cascadedShadowMaps;

    static std::vector<PointLight*> m_pointLights;
    static PointLightShadowMap* m_pointShadowMap;

    static std::vector<SpotLight*> m_spotLights;

    static constexpr uint32_t CLUSTER_DIM_X = 16;
    static constexpr uint32_t CLUSTER_DIM_Y = 9;
    static constexpr uint32_t CLUSTER_DIM_Z = 24;
    static constexpr uint32_t NUM_CLUSTERS = CLUSTER_DIM_X * CLUSTER_DIM_Y * CLUSTER_DIM_Z;
    static constexpr uint32_t MAX_LIGHTS_PER_CLUSTER = 100;
    static constexpr uint32_t CSM_RESOLUTION = 2048;
    static constexpr uint32_t MAX_SHADOWED_POINT_LIGHTS = 8;
    static constexpr uint32_t POINT_SHADOW_RES = 1024;

    static UniformBuffer* m_globalLightUbo;            // directional and light info in UBO: binding 1
    static ShaderStorageBuffer* m_pointLightSsbo;      // point lights in SSBO: binding 2
    static ShaderStorageBuffer* m_spotLightSsbo;       // spot lights in SSBO: binding 3
    static ShaderStorageBuffer* m_clusterAabbSsbo;     // cluster AABBs dimensions in SSBO: binding 4
    static ShaderStorageBuffer* m_lightIndexSsbo;      // light index list in SSBO: binding 5
    static ShaderStorageBuffer* m_lightGridSsbo;       // light grid in SSBO: binding 6
    static ShaderStorageBuffer* m_globalCountSsbo;     // global index counter in SSBO: binding 7
    static ShaderStorageBuffer* m_pointShadowMetaSsbo; // point light shadows in SSBO: binding 8

    static Shader* m_clusterShader;
    static Shader* m_cullShader;
    static Shader* m_depthShader;
    static Shader* m_shadowDepthShader;
    static Shader* m_pointShadowDepthShader;

    static std::vector<const Object*> m_renderQueue;

    // debug tools
    static Mesh* m_debugClusterMesh;
    static Shader* m_debugClusterShader;
    static uint32_t m_drawCallCount;
    static uint32_t m_shadowDrawCallCount;
    static uint32_t m_culledCount;
    static uint32_t m_triangleCount;
    static uint32_t m_submittedCount;
    static int m_debugMode;
    static int m_debugCascade;
    static std::vector<ShadowedPointLightDebug> m_shadowedPointLightsDebug;
};


#endif //MANGORENDERING_RENDERAPI_H