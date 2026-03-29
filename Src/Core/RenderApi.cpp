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
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
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

    m_clusterSystem = std::make_unique<ClusterSystem>();
    m_shadowRenderer = std::make_unique<ShadowRenderer>();
    m_lightManager = std::make_unique<LightManager>();
}

void RenderApi::InitDepthPass() {
    m_depthShader = std::make_unique<Shader>("../Assets/Shaders/depth_only.vert", "../Assets/Shaders/depth_only.frag");
}

RenderApi::~RenderApi() {
    // destroy all GL resources while the context is still alive
    m_clusterSystem.reset();
    m_lightManager.reset();
    m_shadowRenderer.reset();
    m_depthShader.reset();
    m_cameraUbo.reset();
    m_ibl = {};

    m_windows.clear(); // destroys the window and GL context

    SDL_Quit();
}

std::unique_ptr<Window> RenderApi::CreateWindow(const char* title, const glm::vec2 size, const Uint32 flags) {
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

    std::unique_ptr<Window> raw = std::move(window);
    m_windows.push_back(std::move(window));
    return raw;
}

void RenderApi::HandleResizeEvent(const SDL_Event &event) const {
    if (event.type != SDL_EVENT_WINDOW_RESIZED) {
        return;
    }

    int w, h;
    SDL_GetWindowSizeInPixels(m_windows[0]->GetSDLWindow(), &w, &h);
    glViewport(0, 0, w, h);
}

bool RenderApi::IsVisible(const MeshNode3d *node, const Frustum &frustum, RenderStats& stats) {
    const Mesh* mesh = node->GetMesh();
    const glm::vec3 worldCenter = glm::vec3(node->GetWorldMatrix() * glm::vec4(mesh->GetBoundsCenter(), 1.0f));
    const float worldRadius = mesh->GetBoundsRadius() * std::max({ node->GetScale().x, node->GetScale().y, node->GetScale().z });

    if (!frustum.IntersectsSphere(worldCenter, worldRadius)) {
        stats.culled++;
        return true;
    }

    return false;
}

// expects shaders and materials to be bound already
void RenderApi::SubmitToGpu(const MeshNode3d *node, const Shader *shader, RenderStats& stats) {
    shader->SetMatrix4("u_Model", node->GetWorldMatrix());
    shader->SetMatrix4("u_NormalMatrix", glm::transpose(glm::inverse(node->GetWorldMatrix())));

    node->GetMesh()->GetBuffer()->Bind();
    glDrawElements(GL_TRIANGLES, node->GetMesh()->GetBuffer()->GetIndexCount(), GL_UNSIGNED_INT, 0);

    stats.drawCalls++;
    stats.triangles += node->GetMesh()->GetBuffer()->GetIndexCount() / 3;
}

void RenderApi::UploadCameraData(const CameraNode3d* camera) const {
    if (!camera || !m_cameraUbo) return;
    const glm::mat4 view = camera->GetViewMatrix();
    const glm::mat4 proj = camera->GetProjectionMatrix();
    m_cameraUbo->SetData(&view, sizeof(glm::mat4), 0);
    m_cameraUbo->SetData(&proj, sizeof(glm::mat4), sizeof(glm::mat4));
}

void RenderApi::ApplyMaterialCull(const Material &mat) {
    if (mat.GetDoubleSided()) {
        glDisable(GL_CULL_FACE);
    } else {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    }
}

void RenderApi::ClearColour(const glm::vec4 &colour) {
    glClearColor(colour.r, colour.g, colour.b, colour.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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

void RenderApi::SubmitRenderable(RenderableNode3d* node) {
    if (const auto meshNode = dynamic_cast<MeshNode3d*>(node)) {
        m_meshQueue.push_back(meshNode);
    }
}

void RenderApi::SetSkybox(SkyboxNode3d* skybox) {
    if (m_skybox == skybox) return;
    m_skybox = skybox;

    if (skybox && skybox->GetSkybox()) {
        m_ibl = IBLPrecomputer::Compute(skybox->GetSkybox()->GetTexture());
        m_hasIbl = true;
    } else {
        m_hasIbl = false;
    }
}

RenderStats RenderApi::RenderScene(const CameraNode3d* camera, const Framebuffer* targetFbo) const {
    RenderStats stats = {};
    if (!camera || !targetFbo) {
        return stats;
    }

    std::vector<MeshNode3d*> transparentQueue;
    std::vector<MeshNode3d*> opaqueQueue;

    for (MeshNode3d* node : m_meshQueue) {
        const BlendMode mode = node->GetActiveMaterial().GetBlendMode();
        if (mode == BlendMode::AlphaBlend || mode == BlendMode::Additive) {
            transparentQueue.push_back(node);
            continue;
        }

        opaqueQueue.push_back(node);
    }

    // sort transparent back to front by distance to camera
    const glm::vec3 camPos = camera->GetPosition();
    const glm::vec3 forward = camera->GetFront();
    std::ranges::sort(transparentQueue, [&](const MeshNode3d* a, const MeshNode3d* b) {
        const float depthA = glm::dot(glm::vec3(a->GetWorldMatrix()[3]) - camPos, forward);
        const float depthB = glm::dot(glm::vec3(b->GetWorldMatrix()[3]) - camPos, forward);
        return depthA > depthB;
    });

    stats.submitted = static_cast<uint32_t>(m_meshQueue.size());

    UploadCameraData(camera);
    m_lightManager->Upload();
    m_shadowRenderer->ResetStats();

    RebuildClusters(camera, targetFbo);

    // shadow pass
    const glm::vec2 targetSize(targetFbo->GetWidth(), targetFbo->GetHeight());
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.5f, 3.0f);
    m_shadowRenderer->RenderDirectionalShadows(*camera, opaqueQueue, targetSize);
    m_shadowRenderer->RenderPointLightShadows(*camera, m_lightManager->GetPointLights(), opaqueQueue, targetSize);
    glDisable(GL_POLYGON_OFFSET_FILL);

    stats.shadowDrawCalls = m_shadowRenderer->GetShadowDrawCallCount();

    targetFbo->Bind();
    glViewport(0, 0, targetFbo->GetWidth(), targetFbo->GetHeight());

    glDepthMask(GL_TRUE);
    ClearColour({0.16f, 0.16f, 0.16f, 1.0f});

    // z-prepass
    BeginZPrepass();
    m_depthShader->Bind();
    for (const MeshNode3d* mesh : opaqueQueue) {
        DrawMeshNodeDepth(mesh);
    }
    EndZPrepass();

    // light culling
    RunLightCulling();
    m_shadowRenderer->BindPointShadowTexture();

    RenderMainPass(camera, targetFbo, opaqueQueue, stats);

    if (m_skybox) {
        m_skybox->GetSkybox()->Draw(camera->GetViewMatrix(), camera->GetProjectionMatrix());
    }

    // transparent pass
    RenderTransparentPass(camera, transparentQueue, stats);

    Framebuffer::Unbind();
    return stats;
}

void RenderApi::RenderMainPass(const CameraNode3d* camera, const Framebuffer* targetFbo, const std::vector<MeshNode3d*>& opaqueQueue, RenderStats& stats) const {
    const glm::mat4 viewProj = camera->GetProjectionMatrix() * camera->GetViewMatrix();
    Frustum cameraFrustum{};
    cameraFrustum.ExtractFromMatrix(viewProj);

    auto sortedQueue = opaqueQueue;
    std::ranges::sort(sortedQueue, [](const MeshNode3d* a, const MeshNode3d* b) {
        if (a->GetShader() != b->GetShader())
            return a->GetShader() < b->GetShader();
        return &a->GetActiveMaterial() < &b->GetActiveMaterial();
    });

    const Shader* lastShader = nullptr;
    const Material* lastMaterial = nullptr;

    for (const MeshNode3d* node : sortedQueue) {
        if (IsVisible(node, cameraFrustum, stats)) continue;

        const Shader* currentShader = node->GetShader().get();
        const Material& currentMat = node->GetActiveMaterial();

        if (currentShader != lastShader) {
            currentShader->Bind();

            // set shader global uniforms
            glm::vec2 actualScreenSize(targetFbo->GetWidth(), targetFbo->GetHeight());
            currentShader->SetVector2("u_ScreenSize", actualScreenSize);

            currentShader->SetVector3("u_CameraPos", camera->GetPosition());
            currentShader->SetFloat("u_ZNear", camera->GetNearPlane());
            currentShader->SetFloat("u_ZFar", camera->GetFarPlane());
            currentShader->SetInt("u_DebugMode", m_debugMode);
            currentShader->SetInt("u_DebugCascade", m_debugCascade);

            m_shadowRenderer->BindShadowUniforms(*currentShader);

            if (m_hasIbl) {
                m_ibl.irradiance->Bind(20);
                m_ibl.prefiltered->Bind(21);
                currentShader->SetInt("u_IrradianceMap", 20);
                currentShader->SetInt("u_PrefilteredEnvMap", 21);
                currentShader->SetInt("u_MaxPrefilteredMipLevel", IBLPrecomputer::PREFILTER_MIP_LEVELS);
                currentShader->SetBool("u_HasIbl", true);
                currentShader->SetFloat("u_IblDiffuseIntensity", m_skybox->GetIntensity());
                currentShader->SetFloat("u_IblSpecularIntensity", m_skybox->GetSpecularIntensity());
            } else {
                currentShader->SetBool("u_HasIbl", false);
            }

            lastShader = currentShader;
            lastMaterial = nullptr;
        }

        if (&currentMat != lastMaterial) {
            ApplyMaterialCull(currentMat);
            currentMat.Bind(*currentShader);
            lastMaterial = &currentMat;
        }

        SubmitToGpu(node, currentShader, stats);
    }
}

void RenderApi::RenderTransparentPass(const CameraNode3d* camera, const std::vector<MeshNode3d*>& transparentQueue, RenderStats& stats) const {
    if (transparentQueue.empty()) {
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LEQUAL);
        return;
    }

    const glm::mat4 viewProj = camera->GetProjectionMatrix() * camera->GetViewMatrix();
    Frustum cameraFrustum{};
    cameraFrustum.ExtractFromMatrix(viewProj);

    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LESS);

    const Shader* lastShader = nullptr;
    const Material* lastMaterial = nullptr;

    for (const MeshNode3d* node : transparentQueue) {
        if (IsVisible(node, cameraFrustum, stats)) continue;

        if (node->GetActiveMaterial().GetBlendMode() == BlendMode::Additive) {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        } else {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }

        const Shader* currentShader = node->GetShader().get();
        const Material& currentMat = node->GetActiveMaterial();

        if (currentShader != lastShader) {
            currentShader->Bind();
            lastShader = currentShader;
            lastMaterial = nullptr;
        }

        if (&currentMat != lastMaterial) {
            currentMat.Bind(*currentShader);
            lastMaterial = &currentMat;
        }

        SubmitToGpu(node, currentShader, stats);
    }

    // restore state
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
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
}

void RenderApi::RebuildClusters(const CameraNode3d* camera, const Framebuffer *targetFbo) const {
    m_clusterSystem->Rebuild(*camera, {targetFbo->GetWidth(), targetFbo->GetHeight()});
}

void RenderApi::RunLightCulling() const {
    if (!m_clusterSystem) {
        std::cerr << "RunLightCulling: no cluster system\n";
        return;
    }

    m_clusterSystem->Cull(m_lightManager->GetPointLightSsbo(), m_lightManager->GetSpotLightSsbo());
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void RenderApi::DrawMeshNodeDepth(const MeshNode3d *node) const {
    if (!node || !node->GetMesh()->IsUploaded()) {
        std::cerr << "DrawObjectDepth called with null Object/Mesh" << std::endl;
        return;
    }

    const Material& mat = node->GetActiveMaterial();
    ApplyMaterialCull(mat);

    m_depthShader->SetMatrix4("u_Model", node->GetWorldMatrix());

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

void RenderApi::SetDebugMode(const int mode) { m_debugMode = mode; }
void RenderApi::SetDebugCascade(const int cascade) { m_debugCascade = cascade; }
