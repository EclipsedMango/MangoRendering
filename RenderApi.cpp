#include "RenderApi.h"

#include <algorithm>
#include <SDL_video.h>
#include <stdexcept>

#include "GpuLights.h"
#include "UniformBuffer.h"
#include "glad/gl.h"
#include "glm/gtc/type_ptr.hpp"

bool RenderApi::m_gladInitialized = false;
Camera* RenderApi::m_activeCamera {};
UniformBuffer* RenderApi::m_cameraUbo {};
std::vector<Window*> RenderApi::m_windows {};

std::vector<DirectionalLight*> RenderApi::m_directionalLights;
std::vector<PointLight*> RenderApi::m_pointLights;
std::vector<SpotLight*> RenderApi::m_spotLights;

UniformBuffer* RenderApi::m_globalLightUbo = nullptr;
ShaderStorageBuffer* RenderApi::m_pointLightSsbo = nullptr;
ShaderStorageBuffer* RenderApi::m_spotLightSsbo = nullptr;

constexpr uint32_t MAX_TEXTURE_SLOTS = 16;
constexpr uint32_t MAX_DIR_LIGHTS = 4;

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
        throw std::runtime_error("Null Directional light");
    }

    if (m_directionalLights.size() >= MAX_DIR_LIGHTS) {
        throw std::runtime_error("Max Directional Lights reached");
    }

    m_directionalLights.push_back(light);
}

void RenderApi::RemoveDirectionalLight(DirectionalLight *light) {
    std::erase(m_directionalLights, light);
}

void RenderApi::AddPointLight(PointLight *light) {
    if (light) {
        m_pointLights.push_back(light);
    }
}

void RenderApi::RemovePointLight(PointLight *light) {
    std::erase(m_pointLights, light);
}

void RenderApi::AddSpotLight(SpotLight *light) {
    if (light) {
        m_spotLights.push_back(light);
    }
}

void RenderApi::RemoveSpotLight(SpotLight *light) {
    std::erase(m_spotLights, light);
}

void RenderApi::UploadLightData() {
    // layout (counts + directional lights) - binding 1
    // 16 bytes: ivec4 counts (x=dir, y=point, z=spot, w=pad)
    // 32 bytes * MAX_DIR_LIGHTS: directional light array
    constexpr size_t uboSize = sizeof(glm::ivec4) + (MAX_DIR_LIGHTS * 2 * sizeof(glm::vec4));
    if (!m_globalLightUbo) {
        m_globalLightUbo = new UniformBuffer(uboSize, 1);
    }

    const glm::ivec4 counts(
        static_cast<int>(m_directionalLights.size()),
        static_cast<int>(m_pointLights.size()),
        static_cast<int>(m_spotLights.size()),
        0
    );
    m_globalLightUbo->SetData(&counts, sizeof(glm::ivec4), 0);

    size_t offset = sizeof(glm::ivec4);
    for (size_t i = 0; i < MAX_DIR_LIGHTS; i++) {
        GPUDirectionalLight gpuLight{};

        if (i < m_directionalLights.size()) {
            const DirectionalLight* l = m_directionalLights[i];
            gpuLight.direction = glm::vec4(l->GetDirection(), 0.0f);
            gpuLight.color = glm::vec4(l->GetColor(), l->GetIntensity());
        } else {
            // unused slots
            gpuLight.direction = glm::vec4(0.0f);
            gpuLight.color = glm::vec4(0.0f);
        }

        m_globalLightUbo->SetData(&gpuLight, sizeof(GPUDirectionalLight), offset);
        offset += sizeof(GPUDirectionalLight);
    }

    // point lights - binding 2
    if (!m_pointLights.empty()) {
        std::vector<GPUPointLight> pointData;
        pointData.reserve(m_pointLights.size());

        for (auto* l : m_pointLights) {
            GPUPointLight gl {};
            gl.position = glm::vec4(l->GetPosition(), 1.0f);
            gl.color = glm::vec4(l->GetColor(), l->GetIntensity());
            gl.attenuation = glm::vec4(l->GetConstant(), l->GetLinear(), l->GetQuadratic(), 0.0f);
            pointData.push_back(gl);
        }

        size_t size = pointData.size() * sizeof(GPUPointLight);
        // reallocate if null or too small
        if (!m_pointLightSsbo || m_pointLightSsbo->GetSize() < size) {
            delete m_pointLightSsbo;
            m_pointLightSsbo = new ShaderStorageBuffer(size, 2);
        }

        m_pointLightSsbo->SetData(pointData.data(), size);
    }

    // spot lights - binding 3
    if (!m_spotLights.empty()) {
        std::vector<GPUSpotLight> spotData;
        spotData.reserve(m_spotLights.size());

        for (auto* l : m_spotLights) {
            GPUSpotLight gl {};
            gl.position = glm::vec4(l->GetPosition(), 1.0f);
            gl.direction = glm::vec4(l->GetDirection(), 0.0f);
            gl.color = glm::vec4(l->GetColor(), l->GetIntensity());
            gl.params = glm::vec4(l->GetCutOff(), l->GetOuterCutOff(), l->GetLinear(), l->GetQuadratic());
            spotData.push_back(gl);
        }

        size_t size = spotData.size() * sizeof(GPUSpotLight);
        // reallocate if null or too small
        if (!m_spotLightSsbo || m_spotLightSsbo->GetSize() < size) {
            delete m_spotLightSsbo;
            m_spotLightSsbo = new ShaderStorageBuffer(size, 3);
        }

        m_spotLightSsbo->SetData(spotData.data(), size);
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
