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

    static VertexArray* CreateBuffer(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

    static void AddDirectionalLight(DirectionalLight* light);
    static void RemoveDirectionalLight(DirectionalLight* light);

    static void AddPointLight(PointLight* light);
    static void RemovePointLight(PointLight* light);

    static void AddSpotLight(SpotLight* light);
    static void RemoveSpotLight(SpotLight* light);

    static void UploadLightData();

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

    static UniformBuffer* m_globalLightUbo;        // directional and light info in UBO: binding 1
    static ShaderStorageBuffer* m_pointLightSsbo;  // point lights in SSBO: binding 2
    static ShaderStorageBuffer* m_spotLightSsbo;   // spot lights in SSBO: binding 3
};


#endif //MANGORENDERING_RENDERAPI_H