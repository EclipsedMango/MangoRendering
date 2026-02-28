#include "RenderApi.h"

#include <algorithm>
#include <iostream>
#include <ostream>
#include <SDL3/SDL_video.h>
#include <stdexcept>

#include "../Lights/GpuLights.h"
#include "../Buffers/UniformBuffer.h"
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

ShaderStorageBuffer* RenderApi::m_clusterAabbSsbo = nullptr;
ShaderStorageBuffer* RenderApi::m_lightIndexSsbo = nullptr;
ShaderStorageBuffer* RenderApi::m_lightGridSsbo = nullptr;
ShaderStorageBuffer* RenderApi::m_globalCountSsbo = nullptr;

Shader* RenderApi::m_clusterShader = nullptr;
Shader* RenderApi::m_cullShader = nullptr;
Shader* RenderApi::m_depthShader = nullptr;

Mesh* RenderApi::m_debugClusterMesh = nullptr;
Shader* RenderApi::m_debugClusterShader = nullptr;

constexpr uint32_t MAX_TEXTURE_SLOTS = 16;
constexpr uint32_t MAX_DIR_LIGHTS = 4;

void RenderApi::Init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
}

Window* RenderApi::CreateWindow(const char* title, const glm::vec2 size, const Uint32 flags) {
    Window* window = new Window(title, size, flags);
    window->MakeCurrent();

    if (!m_gladInitialized) {
        if (!gladLoadGL(reinterpret_cast<GLADloadfunc>(SDL_GL_GetProcAddress))) {
            throw std::runtime_error("Failed to initialize Glad");
        }

        m_gladInitialized = true;

        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback([](const GLenum source, const GLenum type, const GLuint id, const GLenum severity, GLsizei, const GLchar* message, const void*) {
            if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) return;

            const char* srcStr   = source   == GL_DEBUG_SOURCE_API             ? "API"
                                 : source   == GL_DEBUG_SOURCE_SHADER_COMPILER ? "Shader Compiler"
                                 : source   == GL_DEBUG_SOURCE_APPLICATION     ? "Application"
                                 : "Other";

            const char* typeStr  = type     == GL_DEBUG_TYPE_ERROR               ? "ERROR"
                                 : type     == GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR ? "DEPRECATED"
                                 : type     == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR  ? "UNDEFINED BEHAVIOUR"
                                 : type     == GL_DEBUG_TYPE_PERFORMANCE         ? "PERFORMANCE"
                                 : "OTHER";

            const char* sevStr = severity == GL_DEBUG_SEVERITY_HIGH ? "HIGH" : severity == GL_DEBUG_SEVERITY_MEDIUM ? "MEDIUM" : "LOW";

            std::cerr << "[GL " << typeStr << "] [" << sevStr << "] (" << srcStr << ") id=" << id << "\n  " << message << "\n";
        }, nullptr);

        m_clusterShader = new Shader("../Assets/Shaders/cluster_build.comp");
        m_cullShader    = new Shader("../Assets/Shaders/light_cull.comp");
        InitDepthPass();

        constexpr size_t aabbSize  = NUM_CLUSTERS * 2 * sizeof(glm::vec4);
        constexpr size_t indexSize = NUM_CLUSTERS * MAX_LIGHTS_PER_CLUSTER * sizeof(uint32_t);
        constexpr size_t gridSize  = NUM_CLUSTERS * sizeof(glm::uvec4);

        m_clusterAabbSsbo  = new ShaderStorageBuffer(aabbSize, 4);
        m_lightIndexSsbo   = new ShaderStorageBuffer(indexSize, 5);
        m_lightGridSsbo    = new ShaderStorageBuffer(gridSize, 6);
        m_globalCountSsbo  = new ShaderStorageBuffer(sizeof(uint32_t),7);
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    m_windows.push_back(window);
    return window;
}

void RenderApi::ClearColour(const glm::vec4 &colour) {
    glClearColor(colour.r, colour.g, colour.b, colour.a);
    glClear(GL_COLOR_BUFFER_BIT);
}

void RenderApi::HandleResizeEvent(const SDL_Event &event) {
    if (event.type == SDL_EVENT_WINDOW_RESIZED) {
        const int width = event.window.data1;
        const int height = event.window.data2;
        glViewport(0, 0, width, height);
        if (m_activeCamera) {
            m_activeCamera->SetAspectRatio(static_cast<float>(width) / static_cast<float>(height));
            RebuildClusters();
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

void RenderApi::InitDepthPass() {
    m_depthShader = new Shader("../Assets/Shaders/depth_only.vert", "../Assets/Shaders/depth_only.frag");
}

void RenderApi::DrawObjectDepth(const Object *object) {
    if (!object || !object->GetMesh()->IsUploaded()) {
        std::cerr << "DrawObjectDepth called with null Object/Mesh" << std::endl;
        return;
    }

    m_depthShader->Bind();
    m_depthShader->SetMatrix4("u_Model", object->transform.GetModelMatrix());

    object->GetMesh()->GetBuffer()->Bind();
    glDrawElements(GL_TRIANGLES, object->GetMesh()->GetBuffer()->GetIndexCount(), GL_UNSIGNED_INT, 0);
}

void RenderApi::BeginZPrepass() {
    // disable writing to the color buffer
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);

    // glEnable(GL_POLYGON_OFFSET_FILL);
    // glPolygonOffset(1.0f, 1.0f);

    glClear(GL_DEPTH_BUFFER_BIT);
}

void RenderApi::EndZPrepass() {
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);

    // glDisable(GL_POLYGON_OFFSET_FILL);

    // glMemoryBarrier(GL_DEPTH_BUFFER_BIT);

    // transparent objs go here with glDepthMask(GL_FALSE) and glDepthFunc(GL_LESS)
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

            float radius = CalculateLightRadius(
                l->GetColor(),
                l->GetIntensity(),
                l->GetConstant(),
                l->GetLinear(),
                l->GetQuadratic()
            );

            gl.position = glm::vec4(l->GetPosition(), radius);
            gl.color = glm::vec4(l->GetColor(), l->GetIntensity());
            gl.attenuation = glm::vec4(l->GetConstant(), l->GetLinear(), l->GetQuadratic(), 0.0f);
            pointData.push_back(gl);
        }

        size_t size = pointData.size() * sizeof(GPUPointLight);
        if (!m_pointLightSsbo || m_pointLightSsbo->GetSize() < size) {
            delete m_pointLightSsbo;
            m_pointLightSsbo = new ShaderStorageBuffer(size, 2);
        }

        m_pointLightSsbo->SetData(pointData.data(), size, 0);
    }

    // spot lights - binding 3
    if (!m_spotLights.empty()) {
        std::vector<GPUSpotLight> spotData;
        spotData.reserve(m_spotLights.size());

        for (auto* l : m_spotLights) {
            GPUSpotLight gl {};

            float radius = CalculateLightRadius(
                l->GetColor(),
                l->GetIntensity(),
                1.0f,
                l->GetLinear(),
                l->GetQuadratic()
            );

            gl.position = glm::vec4(l->GetPosition(), radius);
            gl.direction = glm::vec4(l->GetDirection(), 0.0f);
            gl.color = glm::vec4(l->GetColor(), l->GetIntensity());
            gl.params = glm::vec4(l->GetCutOff(), l->GetOuterCutOff(), l->GetLinear(), l->GetQuadratic());
            spotData.push_back(gl);
        }

        size_t size = spotData.size() * sizeof(GPUSpotLight);
        if (!m_spotLightSsbo || m_spotLightSsbo->GetSize() < size) {
            delete m_spotLightSsbo;
            m_spotLightSsbo = new ShaderStorageBuffer(size, 3);
        }

        m_spotLightSsbo->SetData(spotData.data(), size, 0);
    }
}

void RenderApi::RebuildClusters() {
    if (!m_clusterShader || !m_activeCamera) {
        std::cerr << "No cluster shader or active camera" << std::endl;
        return;
    }

    m_clusterShader->Bind();

    m_clusterShader->SetMatrix4("u_InverseProjection", glm::inverse(m_activeCamera->GetProjectionMatrix()));

    // replace with RenderApi::GetActiveWindowSize()
    const glm::vec2 winSize = m_windows[0]->GetSize();
    m_clusterShader->SetVector2("u_ScreenDimensions", winSize);

    m_clusterShader->SetFloat("u_ZNear", m_activeCamera->GetNearPlane());
    m_clusterShader->SetFloat("u_ZFar", m_activeCamera->GetFarPlane());

    if (m_clusterAabbSsbo) {
        m_clusterAabbSsbo->Bind();
    }

    m_clusterShader->Dispatch(CLUSTER_DIM_X, CLUSTER_DIM_Y, CLUSTER_DIM_Z);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void RenderApi::RunLightCulling() {
    if (!m_cullShader || !m_activeCamera) {
        std::cerr << "No cull shader or active camera" << std::endl;
        return;
    }

    m_cullShader->Bind();

    // reset counter
    constexpr uint32_t zero = 0;
    m_globalCountSsbo->Bind();
    glClearNamedBufferData(m_globalCountSsbo->GetId(), GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);

    // reset light grid counts
    m_lightGridSsbo->Bind();
    glClearNamedBufferData(m_lightGridSsbo->GetId(), GL_RGBA32UI, GL_RGBA_INTEGER, GL_UNSIGNED_INT, nullptr);

    m_clusterAabbSsbo->Bind();
    m_lightIndexSsbo->Bind();
    if (m_pointLightSsbo) m_pointLightSsbo->Bind();
    if (m_spotLightSsbo) m_spotLightSsbo->Bind();

    m_cullShader->Dispatch(CLUSTER_DIM_X, CLUSTER_DIM_Y, CLUSTER_DIM_Z);

    // SUPER IMPORTANT: wait for writes to finish before the frag reads them
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

float RenderApi::CalculateLightRadius(const glm::vec3 &color, const float intensity, const float constant, float linear, float quadratic) {
    constexpr float threshold = 0.005f;

    const float maxChannel = std::max(std::max(color.r, color.g), color.b);
    const float maxBrightness = maxChannel * intensity;

    if (maxBrightness <= 0.0f) {
        return 0.0f;
    }

    // if quadratic is pretty much zero, solve linear: I / (C + L*d) = T
    if (quadratic < 0.0001f) {
        if (linear < 0.0001f) {
            return 1000.0f;
        }

        return (maxBrightness / threshold - constant) / linear;
    }

    // solve quadratic: I / (C + L*d + Q*d^2) = T
    const float val = (maxBrightness / threshold - constant) / quadratic;
    return val > 0.0f ? std::sqrt(val) : 0.0f;
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

    if (m_activeCamera) {
        object->GetShader()->SetFloat("u_ZNear", m_activeCamera->GetNearPlane());
        object->GetShader()->SetFloat("u_ZFar", m_activeCamera->GetFarPlane());
    }

    if (!m_windows.empty()) {
        const glm::vec2 size = m_windows[0]->GetSize();
        object->GetShader()->SetVector2("u_ScreenSize", size);
    }

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

void RenderApi::DrawClusterVisualizer() {
    if (!m_debugClusterMesh) {
        const std::vector<Vertex> vertices = {
            {{0, 0, 0}, {0,0,0}, {0,0}}, {{1, 0, 0}, {0,0,0}, {0,0}},
            {{1, 1, 0}, {0,0,0}, {0,0}}, {{0, 1, 0}, {0,0,0}, {0,0}},
            {{0, 0, 1}, {0,0,0}, {0,0}}, {{1, 0, 1}, {0,0,0}, {0,0}},
            {{1, 1, 1}, {0,0,0}, {0,0}}, {{0, 1, 1}, {0,0,0}, {0,0}}
        };

        const std::vector<uint32_t> indices = {
            0,1, 1,2, 2,3, 3,0,  // bottom face
            4,5, 5,6, 6,7, 7,4,  // top face
            0,4, 1,5, 2,6, 3,7   // verticals
        };

        m_debugClusterMesh = new Mesh(vertices, indices);
        m_debugClusterMesh->Upload();
        m_debugClusterShader = new Shader("../Assets/Shaders/debug_clusters.vert", "../Assets/Shaders/debug_clusters.frag");
    }

    if (!m_activeCamera || !m_clusterAabbSsbo) return;

    m_debugClusterShader->Bind();

    m_debugClusterShader->SetMatrix4("u_View", m_activeCamera->GetViewMatrix());
    m_debugClusterShader->SetMatrix4("u_Projection", m_activeCamera->GetProjectionMatrix());

    m_debugClusterShader->SetUint("u_DimX", CLUSTER_DIM_X);
    m_debugClusterShader->SetUint("u_DimY", CLUSTER_DIM_Y);
    m_debugClusterShader->SetUint("u_DimZ", CLUSTER_DIM_Z);

    m_clusterAabbSsbo->Bind();
    m_debugClusterMesh->GetBuffer()->Bind();

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDisable(GL_CULL_FACE);

    glDrawElementsInstanced(GL_LINES, m_debugClusterMesh->GetBuffer()->GetIndexCount(), GL_UNSIGNED_INT, nullptr, NUM_CLUSTERS);

    glEnable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
