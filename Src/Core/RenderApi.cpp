#include "RenderApi.h"

#include <algorithm>
#include <iostream>
#include <ostream>
#include <SDL3/SDL_video.h>
#include <stdexcept>

#include "Renderer/Lights/GpuLights.h"
#include "Renderer/Buffers/UniformBuffer.h"
#include "glad/gl.h"
#include "glm/gtc/type_ptr.hpp"
#include "Nodes/MeshNode3d.h"
#include "Scene/Frustum.h"

void RenderApi::InitSDL() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
}

RenderApi::~RenderApi() {
    // destroy all GL resources while the context is still alive
    m_clusterSystem.reset();
    m_lightManager.reset();
    m_shadowRenderer.reset();
    m_depthShader.reset();
    m_cameraUbo.reset();
    m_debugClusterMesh.reset();
    m_debugClusterShader.reset();
    m_ibl = {};
    m_skybox = nullptr;

    m_windows.clear(); // destroys the window and GL context

    SDL_Quit();
}

Window* RenderApi::CreateWindow(const char* title, const glm::vec2 size, const Uint32 flags) {
    auto window = std::make_unique<Window>(title, size, flags);
    window->MakeCurrent();

    if (m_windows.empty()) {
        if (!gladLoadGL(SDL_GL_GetProcAddress)) {
            throw std::runtime_error("Failed to initialize Glad");
        }

        InitGLResources();
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    Window* raw = window.get();
    m_windows.push_back(std::move(window));
    return raw;
}

void RenderApi::InitGLResources() {
    // debug output
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback([](const GLenum source, const GLenum type, const GLuint id, const GLenum severity, GLsizei, const GLchar* message, const void*) {
        if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) return;

        const char* srcStr  = source == GL_DEBUG_SOURCE_API             ? "API"
                            : source == GL_DEBUG_SOURCE_SHADER_COMPILER ? "Shader Compiler"
                            : source == GL_DEBUG_SOURCE_APPLICATION     ? "Application"
                            : "Other";

        const char* typeStr = type == GL_DEBUG_TYPE_ERROR               ? "ERROR"
                            : type == GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR ? "DEPRECATED"
                            : type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR  ? "UNDEFINED BEHAVIOUR"
                            : type == GL_DEBUG_TYPE_PERFORMANCE         ? "PERFORMANCE"
                            : "OTHER";

        const char* sevStr  = severity == GL_DEBUG_SEVERITY_HIGH   ? "HIGH"
                            : severity == GL_DEBUG_SEVERITY_MEDIUM ? "MEDIUM"
                            : "LOW";

        std::cerr << "[GL " << typeStr << "] [" << sevStr << "] (" << srcStr << ") id=" << id << "\n  " << message << "\n";
    }, nullptr);

    // depth/shadow shaders and point shadow map
    InitDepthPass();

    // camera UBO
    m_cameraUbo = std::make_unique<UniformBuffer>(2 * sizeof(glm::mat4), 0);

    m_clusterSystem  = std::make_unique<ClusterSystem>();
    m_shadowRenderer = std::make_unique<ShadowRenderer>();
    m_lightManager   = std::make_unique<LightManager>();
}

void RenderApi::InitDepthPass() {
    m_depthShader = std::make_unique<Shader>("../Assets/Shaders/depth_only.vert", "../Assets/Shaders/depth_only.frag");
}

void RenderApi::ClearColour(const glm::vec4 &colour) {
    glClearColor(colour.r, colour.g, colour.b, colour.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void RenderApi::HandleResizeEvent(const SDL_Event &event) const {
    if (event.type != SDL_EVENT_WINDOW_RESIZED) {
        return;
    }

    int w, h;
    SDL_GetWindowSizeInPixels(m_windows[0]->GetSDLWindow(), &w, &h);
    glViewport(0, 0, w, h);

    if (m_activeCamera) {
        m_activeCamera->SetAspectRatio(static_cast<float>(w) / static_cast<float>(h));
        RebuildClusters();
    }
}

void RenderApi::SetActiveCamera(CameraNode3d* camera) {
    m_activeCamera = camera;

    if (!m_activeCamera || m_windows.empty()) {
        return;
    }

    int w = 1, h = 1;
    SDL_GetWindowSizeInPixels(m_windows[0]->GetSDLWindow(), &w, &h);
    if (h == 0) {
        h = 1;
    }

    m_activeCamera->SetAspectRatio(static_cast<float>(w) / static_cast<float>(h));

    RebuildClusters();
}

void RenderApi::UploadCameraData() const {
    if (!m_activeCamera || !m_cameraUbo) {
        return;
    }

    const glm::mat4 view = m_activeCamera->GetViewMatrix();
    const glm::mat4 proj = m_activeCamera->GetProjectionMatrix();

    m_cameraUbo->SetData(&view, sizeof(glm::mat4), 0);
    m_cameraUbo->SetData(&proj, sizeof(glm::mat4), sizeof(glm::mat4));
}

void RenderApi::AddDirectionalLight(DirectionalLight *light) const {
    m_lightManager->AddDirectionalLight(light);
    m_shadowRenderer->AddDirectionalLight(light);
}

void RenderApi::RemoveDirectionalLight(DirectionalLight *light) const {
    m_lightManager->RemoveDirectionalLight(light);
    m_shadowRenderer->RemoveDirectionalLight(light);
}

void RenderApi::AddPointLight(PointLight* light) const { m_lightManager->AddPointLight(light); }
void RenderApi::RemovePointLight(PointLight* light) const { m_lightManager->RemovePointLight(light); }
void RenderApi::AddSpotLight(SpotLight* light) const { m_lightManager->AddSpotLight(light); }
void RenderApi::RemoveSpotLight(SpotLight* light) const { m_lightManager->RemoveSpotLight(light); }

void RenderApi::SetSkybox(SkyboxNode3d *skybox) {
    m_skybox = skybox;

    if (skybox) {
        m_ibl = IBLPrecomputer::Compute(skybox->GetSkybox().GetTexture());
        m_hasIbl = true;
    } else {
        m_hasIbl = false;
    }
}

void RenderApi::Flush() {
    m_stats = {};

    // partition into opaque and transparent
    m_transparentQueue.clear();
    std::vector<MeshNode3d*> opaqueQueue;
    for (MeshNode3d* node : m_meshQueue) {
        const BlendMode mode = node->GetActiveMaterial().GetBlendMode();
        if (mode == BlendMode::AlphaBlend || mode == BlendMode::Additive) {
            m_transparentQueue.push_back(node);
            continue;
        }

        opaqueQueue.push_back(node);
    }

    // sort transparent back to front by distance to camera
    const glm::vec3 camPos = m_activeCamera->GetPosition();
    std::ranges::sort(m_transparentQueue, [&](const MeshNode3d* a, const MeshNode3d* b) {
        const float da = glm::length(glm::vec3(a->GetWorldMatrix()[3]) - camPos);
        const float db = glm::length(glm::vec3(b->GetWorldMatrix()[3]) - camPos);
        return da > db; // furthest first
    });

    m_stats.submitted = static_cast<uint32_t>(m_meshQueue.size());

    UploadCameraData();
    m_lightManager->Upload();
    m_shadowRenderer->ResetStats();

    // shadow pass
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.5f, 4.0f);
    glCullFace(GL_FRONT);
    m_shadowRenderer->RenderDirectionalShadows(*m_activeCamera, opaqueQueue, m_windows[0]->GetSize());
    glCullFace(GL_BACK);
    glDisable(GL_POLYGON_OFFSET_FILL);
    m_shadowRenderer->RenderPointLightShadows(*m_activeCamera, m_lightManager->GetPointLights(), opaqueQueue, m_windows[0]->GetSize());

    m_stats.shadowDrawCalls = m_shadowRenderer->GetShadowDrawCallCount();

    // z-prepass
    BeginZPrepass();
    for (const MeshNode3d* mesh : opaqueQueue) {
        DrawMeshNodeDepth(mesh);
    }
    EndZPrepass();

    // light culling
    RunLightCulling();

    m_shadowRenderer->BindCSMTextures();
    m_shadowRenderer->BindPointShadowTexture();

    m_meshQueue = opaqueQueue;
    RenderMainPass();

    if (m_skybox) {
        m_skybox->GetSkybox().Draw(m_activeCamera->GetViewMatrix(), m_activeCamera->GetProjectionMatrix());
    }

    // transparent pass
    RenderTransparentPass();

    m_meshQueue.clear();
    m_transparentQueue.clear();
}

void RenderApi::RenderMainPass() {
    const glm::mat4 viewProj = m_activeCamera->GetProjectionMatrix() * m_activeCamera->GetViewMatrix();
    Frustum cameraFrustum{};
    cameraFrustum.ExtractFromMatrix(viewProj);

    for (const MeshNode3d* node : m_meshQueue) {
        const Mesh* mesh = node->GetMesh();
        const glm::vec3 worldCenter = glm::vec3(node->GetWorldMatrix() * glm::vec4(mesh->GetBoundsCenter(), 1.0f));
        const float worldRadius = mesh->GetBoundsRadius() * std::max({ node->GetScale().x, node->GetScale().y, node->GetScale().z });

        if (!cameraFrustum.IntersectsSphere(worldCenter, worldRadius)) {
            m_stats.culled++;
            continue;
        }

        DrawMeshNode(node);
    }
}

void RenderApi::RenderTransparentPass() {
    if (m_transparentQueue.empty()) return;

    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);      // dont write depth
    glDepthFunc(GL_LESS);       // test against opaque depth
    glDisable(GL_CULL_FACE);    // see both sides of transparent surfaces

    const glm::mat4 viewProj = m_activeCamera->GetProjectionMatrix() * m_activeCamera->GetViewMatrix();
    Frustum cameraFrustum{};
    cameraFrustum.ExtractFromMatrix(viewProj);

    for (const MeshNode3d* node : m_transparentQueue) {
        if (node->GetActiveMaterial().GetBlendMode() == BlendMode::Additive) {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        } else {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }

        const Mesh* mesh = node->GetMesh();
        const glm::vec3 worldCenter = glm::vec3(node->GetWorldMatrix() * glm::vec4(mesh->GetBoundsCenter(), 1.0f));
        const float worldRadius = mesh->GetBoundsRadius() * std::max({ node->GetScale().x, node->GetScale().y, node->GetScale().z });

        if (!cameraFrustum.IntersectsSphere(worldCenter, worldRadius)) {
            m_stats.culled++;
            continue;
        }

        DrawMeshNode(node);
    }

    // restore state
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);
}

void RenderApi::BeginZPrepass() {
    // disable writing to the color buffer
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
}

void RenderApi::EndZPrepass() {
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);

    // transparent objs go here with glDepthMask(GL_FALSE) and glDepthFunc(GL_LESS)
}

void RenderApi::RebuildClusters() const {
    if (!m_clusterSystem || !m_activeCamera) {
        std::cerr << "RebuildClusters: no cluster system or active camera\n";
        return;
    }

    m_clusterSystem->Rebuild(*m_activeCamera, m_windows[0]->GetSize());
}

void RenderApi::RunLightCulling() const {
    if (!m_clusterSystem || !m_activeCamera) {
        std::cerr << "RunLightCulling: no cluster system or active camera\n";
        return;
    }

    m_clusterSystem->Cull(m_lightManager->GetPointLightSsbo(), m_lightManager->GetSpotLightSsbo());
}

void RenderApi::DrawMeshNode(const MeshNode3d* node) {
    if (!node) throw std::runtime_error("DrawObject: null object");
    if (!node->GetMesh()->IsUploaded()) throw std::runtime_error("DrawObject: mesh not uploaded to GPU");

    const Shader* shader = node->GetShader();
    shader->Bind();

    shader->SetInt("u_DebugMode", m_debugMode);
    shader->SetInt("u_DebugCascade", m_debugCascade);

    m_shadowRenderer->BindShadowUniforms(*shader);

    shader->SetMatrix4("u_Model", node->GetWorldMatrix());
    shader->SetMatrix4("u_NormalMatrix", glm::transpose(glm::inverse(node->GetWorldMatrix())));

    if (m_activeCamera) {
        shader->SetFloat("u_ZNear", m_activeCamera->GetNearPlane());
        shader->SetFloat("u_ZFar", m_activeCamera->GetFarPlane());
    }

    shader->SetVector2("u_ScreenSize", m_windows[0]->GetSize());
    shader->SetVector3("u_CameraPos", m_activeCamera->GetPosition());

    node->GetActiveMaterial().Bind(*shader);

    if (m_hasIbl) {
        m_ibl.irradiance->Bind(10);
        m_ibl.prefiltered->Bind(11);
        m_ibl.brdfLut->Bind(12);
        shader->SetInt("u_IrradianceMap", 10);
        shader->SetInt("u_PrefilteredEnvMap", 11);
        shader->SetInt("u_BrdfLut", 12);
        shader->SetInt("u_MaxPrefilteredMipLevel", IBLPrecomputer::PREFILTER_MIP_LEVELS);
        shader->SetBool("u_HasIbl", true);

        // TODO: make these values changable, or move to the skybox or something / env node idk
        shader->SetFloat("u_IblDiffuseIntensity", 0.5f);
        shader->SetFloat("u_IblSpecularIntensity", 0.5f);
    } else {
        shader->SetBool("u_HasIbl", false);
    }

    node->GetMesh()->GetBuffer()->Bind();
    glDrawElements(GL_TRIANGLES, node->GetMesh()->GetBuffer()->GetIndexCount(), GL_UNSIGNED_INT, 0);

    m_stats.drawCalls++;
    m_stats.triangles += node->GetMesh()->GetBuffer()->GetIndexCount() / 3;
}

void RenderApi::DrawMeshNodeDepth(const MeshNode3d *node) const {
    if (!node || !node->GetMesh()->IsUploaded()) {
        std::cerr << "DrawObjectDepth called with null Object/Mesh" << std::endl;
        return;
    }

    m_depthShader->Bind();
    m_depthShader->SetMatrix4("u_Model", node->GetWorldMatrix());

    const Material& mat = node->GetActiveMaterial();
    m_depthShader->SetBool("u_AlphaScissor", mat.GetBlendMode() == BlendMode::AlphaScissor);
    m_depthShader->SetFloat("u_AlphaScissorThreshold", mat.GetAlphaScissorThreshold());
    m_depthShader->SetBool("u_HasDiffuse", mat.GetDiffuse() != nullptr);
    m_depthShader->SetVector4("u_AlbedoColor", mat.GetAlbedoColor());
    m_depthShader->SetVector2("u_UVScale", mat.GetUVScale());
    m_depthShader->SetVector2("u_UVOffset", mat.GetUVOffset());

    if (mat.GetDiffuse()) {
        mat.GetDiffuse()->Bind(0);
        m_depthShader->SetInt("u_Diffuse", 0);
    }

    node->GetMesh()->GetBuffer()->Bind();
    glDrawElements(GL_TRIANGLES, node->GetMesh()->GetBuffer()->GetIndexCount(), GL_UNSIGNED_INT, 0);
}

void RenderApi::DrawMesh(const Mesh &mesh, const Shader& shader) {
    if (!mesh.IsUploaded()) {
        throw std::runtime_error("DrawMesh: mesh not uploaded to GPU");
    }

    shader.Bind();
    mesh.GetBuffer()->Bind();
    glDrawElements(GL_TRIANGLES, mesh.GetBuffer()->GetIndexCount(), GL_UNSIGNED_INT, nullptr);
}

void RenderApi::DrawClusterVisualizer() {
    if (!m_debugClusterMesh) {
        const std::vector<Vertex> vertices = {
            {{0,0,0},{0,0,0},{0,0}}, {{1,0,0},{0,0,0},{0,0}},
            {{1,1,0},{0,0,0},{0,0}}, {{0,1,0},{0,0,0},{0,0}},
            {{0,0,1},{0,0,0},{0,0}}, {{1,0,1},{0,0,0},{0,0}},
            {{1,1,1},{0,0,0},{0,0}}, {{0,1,1},{0,0,0},{0,0}}
        };
        const std::vector<uint32_t> indices = {
            0,1, 1,2, 2,3, 3,0,   // bottom face
            4,5, 5,6, 6,7, 7,4,   // top face
            0,4, 1,5, 2,6, 3,7    // verticals
        };

        m_debugClusterMesh   = std::make_unique<Mesh>(vertices, indices);
        m_debugClusterMesh->Upload();
        m_debugClusterShader = std::make_unique<Shader>("../Assets/Shaders/debug_clusters.vert", "../Assets/Shaders/debug_clusters.frag");
    }

    if (!m_activeCamera) {
        return;
    }

    m_debugClusterShader->Bind();
    m_debugClusterShader->SetMatrix4("u_View", m_activeCamera->GetViewMatrix());
    m_debugClusterShader->SetMatrix4("u_Projection", m_activeCamera->GetProjectionMatrix());
    m_debugClusterShader->SetUint("u_DimX", CLUSTER_DIM_X);
    m_debugClusterShader->SetUint("u_DimY", CLUSTER_DIM_Y);
    m_debugClusterShader->SetUint("u_DimZ", CLUSTER_DIM_Z);

    m_debugClusterMesh->GetBuffer()->Bind();

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDisable(GL_CULL_FACE);
    glDrawElementsInstanced(GL_LINES, m_debugClusterMesh->GetBuffer()->GetIndexCount(), GL_UNSIGNED_INT, nullptr, NUM_CLUSTERS);
    glEnable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void RenderApi::SetDebugMode(const int mode) { m_debugMode = mode; }
void RenderApi::SetDebugCascade(const int cascade) { m_debugCascade = cascade; }
