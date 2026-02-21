#include "RenderApi.h"

#include <algorithm>
#include <SDL_video.h>
#include <stdexcept>

#include "UniformBuffer.h"
#include "glad/gl.h"
#include "glm/gtc/type_ptr.hpp"

bool RenderApi::m_gladInitialized = false;
Camera* RenderApi::m_activeCamera {};
UniformBuffer* RenderApi::m_cameraUbo {};
std::vector<Window*> RenderApi::m_windows {};
std::vector<DirectionalLight*> RenderApi::m_directionalLights;
UniformBuffer* RenderApi::m_lightUbo;

constexpr uint32_t MAX_TEXTURE_SLOTS = 16;
constexpr uint32_t MAX_DIRECTIONAL_LIGHTS = 4;

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
        const int width = event.window.data1;
        const int height = event.window.data2;
        glViewport(0, 0, width, height);
        if (m_activeCamera) {
            m_activeCamera->SetAspectRatio(static_cast<float>(width) / static_cast<float>(height));
        }
    }
}

void RenderApi::SetActiveCamera(Camera *camera) {
    m_activeCamera = camera;
    if (!m_cameraUbo) {
        m_cameraUbo = new UniformBuffer(2 * sizeof(glm::mat4), 0);
    }
}

// sets the active cameras data into the cameras UBO.
void RenderApi::UploadCameraData() {
    if (!m_activeCamera || !m_cameraUbo) return;

    const glm::mat4 view = m_activeCamera->GetViewMatrix();
    const glm::mat4 proj = m_activeCamera->GetProjectionMatrix();

    m_cameraUbo->SetData(&view, sizeof(glm::mat4), 0);
    m_cameraUbo->SetData(&proj, sizeof(glm::mat4), sizeof(glm::mat4));
}

VertexArray* RenderApi::CreateBuffer(const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices) {
    return new VertexArray(vertices, indices);
}

void RenderApi::AddDirectionalLight(DirectionalLight *light) {
    if (!light) {
        throw std::runtime_error("AddDirectionalLight called with null light");
    }

    if (m_directionalLights.size() >= MAX_DIRECTIONAL_LIGHTS) {
        throw std::runtime_error("Exceeded maximum directional lights (" + std::to_string(MAX_DIRECTIONAL_LIGHTS) + ")");
    }

    if (!m_lightUbo) {
        // array of lights + int for count, padded to 16 bytes
        const size_t size = sizeof(glm::vec4) + MAX_DIRECTIONAL_LIGHTS * sizeof(glm::vec4) * 2;
        m_lightUbo = new UniformBuffer(size, 1);
    }

    m_directionalLights.push_back(light);
}

void RenderApi::RemoveDirectionalLight(DirectionalLight *light) {
    const auto it = std::ranges::find(m_directionalLights, light);
    if (it != m_directionalLights.end()) {
        m_directionalLights.erase(it);
    }
}

void RenderApi::UploadLightData() {
    if (!m_lightUbo || m_directionalLights.empty()) {
        return;
    }

    const glm::ivec4 lightInfo(static_cast<int>(m_directionalLights.size()), 0, 0, 0);
    m_lightUbo->SetData(&lightInfo, sizeof(glm::ivec4), 0);

    size_t offset = sizeof(glm::vec4);
    for (uint32_t i = 0; i < m_directionalLights.size() && i < MAX_DIRECTIONAL_LIGHTS; i++) {
        glm::vec4 direction = glm::vec4(m_directionalLights[i]->GetDirection(), 0.0f);
        glm::vec4 colorIntensity = glm::vec4(m_directionalLights[i]->GetColor(), m_directionalLights[i]->GetIntensity());

        m_lightUbo->SetData(&direction, sizeof(glm::vec4), offset);
        offset += sizeof(glm::vec4);
        m_lightUbo->SetData(&colorIntensity, sizeof(glm::vec4), offset);
        offset += sizeof(glm::vec4);
    }
}

void RenderApi::DrawMesh(const Mesh &mesh, const Shader& shader) {
    if (!mesh.IsUploaded()) {
        throw std::runtime_error("Mesh has not been uploaded to the GPU");
    }

    shader.Bind();
    mesh.GetBuffer()->Bind();
    glDrawElements(GL_TRIANGLES, mesh.GetBuffer()->GetIndexCount(), GL_UNSIGNED_INT, nullptr);
}

void RenderApi::DrawObject(const Object* object) {
    if (object == nullptr) {
        throw std::runtime_error("DrawObject called with null Object");
    }

    if (!object->GetMesh()->IsUploaded()) {
        throw std::runtime_error("Objects mesh has not been uploaded to the GPU");
    }

    object->GetShader()->Bind();
    object->GetShader()->SetMatrix4("u_Model", object->transform.GetModelMatrix());
    object->GetShader()->SetMatrix4("u_NormalMatrix", glm::transpose(glm::inverse(object->transform.GetModelMatrix())));

    const std::vector<Texture*>& textures = object->GetTextures();
    if (textures.size() > MAX_TEXTURE_SLOTS) {
        throw std::runtime_error("Object exceeds maximum texture slots (" + std::to_string(MAX_TEXTURE_SLOTS) + ")");
    }

    for (uint32_t i = 0; i < textures.size(); i++) {
        textures[i]->Bind(i);
        // TODO: change to have specific names, not just a generic array. (similar to godots material system maybe)
        object->GetShader()->SetInt("u_Textures[" + std::to_string(i) + "]", i);
    }

    object->GetMesh()->GetBuffer()->Bind();
    glDrawElements(GL_TRIANGLES, object->GetMesh()->GetBuffer()->GetIndexCount(), GL_UNSIGNED_INT, 0);
}
