#ifndef MANGORENDERING_RENDERAPI_H
#define MANGORENDERING_RENDERAPI_H

#include <SDL.h>
#include <vector>

#include "Camera.h"
#include "GpuBuffer.h"
#include "Mesh.h"
#include "Shader.h"
#include "Window.h"
#include "glm/vec2.hpp"
#include "glm/vec4.hpp"

class RenderApi {
public:
    static void Init();

    static Window* CreateWindow(const char* title, glm::vec2 pos, glm::vec2 size, Uint32 flags);
    static void ClearColour(const glm::vec4& colour);

    static void HandleResizeEvent(const SDL_Event& event);

    static void SetActiveCamera(Camera* camera);
    static void UploadCameraData();

    static GpuBuffer* CreateBuffer(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
    static void DrawMesh(const Mesh& mesh, const Shader& shader);

protected:
    RenderApi() = default;

private:
    static bool m_gladInitialized;

    static Camera* m_activeCamera;
    static unsigned int m_cameraUbo;

    static std::vector<Window*> m_windows;
};


#endif //MANGORENDERING_RENDERAPI_H