#ifndef MANGORENDERING_RENDERAPI_H
#define MANGORENDERING_RENDERAPI_H

#include <SDL.h>
#include <vector>

#include "Window.h"
#include "Camera.h"
#include "DirectionalLight.h"
#include "VertexArray.h"
#include "Mesh.h"
#include "Object.h"
#include "Shader.h"
#include "UniformBuffer.h"
#include "ShaderStorageBuffer.h"
#include "DirectionalLight.h"
#include "PointLight.h"
#include "SpotLight.h"

class RenderApi {
public:
    static void Init();
    static Window* CreateWindow(const char* title, glm::vec2 pos, glm::vec2 size, Uint32 flags);
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

    static void InitDepthPass();
    static void DrawObjectDepth(const Object* object);

    static void BeginZPrepass();
    static void EndZPrepass();

    static void UploadLightData();
    static void RebuildClusters();
    static void RunLightCulling();
    static float CalculateLightRadius(const glm::vec3& color, float intensity, float constant, float linear, float quadratic);

    static VertexArray* CreateBuffer(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

    static void DrawMesh(const Mesh& mesh, const Shader& shader);
    static void DrawObject(const Object* object);

private:
    static bool m_gladInitialized;
    static Camera* m_activeCamera;
    static UniformBuffer* m_cameraUbo;
    static std::vector<Window*> m_windows;

    static std::vector<DirectionalLight*> m_directionalLights;
    static std::vector<PointLight*> m_pointLights;
    static std::vector<SpotLight*> m_spotLights;

    static constexpr uint32_t CLUSTER_DIM_X = 16;
    static constexpr uint32_t CLUSTER_DIM_Y = 9;
    static constexpr uint32_t CLUSTER_DIM_Z = 24;
    static constexpr uint32_t NUM_CLUSTERS = CLUSTER_DIM_X * CLUSTER_DIM_Y * CLUSTER_DIM_Z;
    static constexpr uint32_t MAX_LIGHTS_PER_CLUSTER = 100;

    static UniformBuffer* m_globalLightUbo;        // directional and light info in UBO: binding 1
    static ShaderStorageBuffer* m_pointLightSsbo;  // point lights in SSBO: binding 2
    static ShaderStorageBuffer* m_spotLightSsbo;   // spot lights in SSBO: binding 3
    static ShaderStorageBuffer* m_clusterAabbSsbo; // cluster AABBs dimensions in SSBO: binding 4
    static ShaderStorageBuffer* m_lightIndexSsbo;  // light index list in SSBO: binding 5
    static ShaderStorageBuffer* m_lightGridSsbo;   // light grid in SSBO: binding 6
    static ShaderStorageBuffer* m_globalCountSsbo; // global index counter in SSBO: binding 7

    static Shader* m_clusterShader;
    static Shader* m_cullShader;
    static Shader* m_depthShader;
};


#endif //MANGORENDERING_RENDERAPI_H