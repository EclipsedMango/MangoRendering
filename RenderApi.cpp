#include "RenderApi.h"

#include <SDL_video.h>
#include <stdexcept>
#include "glad/gl.h"
#include "glm/gtc/type_ptr.hpp"

bool RenderApi::m_gladInitialized = false;
Camera* RenderApi::m_activeCamera {};
unsigned int RenderApi::m_cameraUbo {};
std::vector<Window*> RenderApi::m_windows {};

void RenderApi::Init() {
    SDL_SetHint(SDL_HINT_VIDEODRIVER, "x11");
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
}

Window* RenderApi::CreateWindow(const char* title, const glm::vec2 pos, const glm::vec2 size, const Uint32 flags) {
    Window* window = new Window(title, pos, size, flags);
    window->MakeCurrent();

    if (!m_gladInitialized) {
        if (!gladLoadGL(reinterpret_cast<GLADloadfunc>(SDL_GL_GetProcAddress))) {
            throw std::runtime_error("Failed to initialize Glad");
        }

        m_gladInitialized = true;
    }

    glEnable(GL_DEPTH_TEST);
    m_windows.push_back(window);
    return window;
}

void RenderApi::ClearColour(const glm::vec4 &colour) {
    glClearColor(colour.r, colour.g, colour.b, colour.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void RenderApi::HandleResizeEvent(const SDL_Event &event) {
    if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED) {
        glViewport(0, 0, event.window.data1, event.window.data2);
    }
}

void RenderApi::SetActiveCamera(Camera *camera) {
    m_activeCamera = camera;

    unsigned int ubo;
    glGenBuffers(1, &ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);

    m_cameraUbo = ubo;
}

void RenderApi::UploadCameraData() {
    if (!m_activeCamera) {
        throw std::runtime_error("No active camera");
    }

    glBindBuffer(GL_UNIFORM_BUFFER, m_cameraUbo);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(m_activeCamera->GetViewMatrix()));
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(m_activeCamera->GetProjectionMatrix()));
}

GpuBuffer* RenderApi::CreateBuffer(const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices) {
    return new GpuBuffer(vertices, indices);
}

void RenderApi::DrawMesh(const Mesh &mesh, const Shader& shader) {
    if (!mesh.IsUploaded()) {
        throw std::runtime_error("Mesh has not been uploaded to the GPU");
    }

    shader.Bind();
    mesh.GetBuffer()->Bind();
    glDrawElements(GL_TRIANGLES, mesh.GetBuffer()->GetIndexCount(), GL_UNSIGNED_INT, nullptr);
}
