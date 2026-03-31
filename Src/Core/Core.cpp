
#include "Core.h"

#include <iostream>

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl3.h"
#include "Input.h"
#include "RenderApi.h"
#include "Nodes/Lights/DirectionalLightNode3d.h"
#include "Nodes/Lights/PointLightNode3d.h"
#include "Nodes/Lights/SpotLightNode3d.h"
#include <algorithm>

#include "ResourceManager.h"
#include "Editor/EditorStyle.h"
#include "Nodes/PortalNode3d.h"

Core::~Core() {
    m_currentScene.reset();
    m_globalSkybox.reset();
    m_defaultShader.reset();
    m_mainFramebuffer.reset();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    m_renderer.reset();
}

void Core::Init() {
    InitRenderer();
    InitImGui();

    if (m_currentScene) {
        m_currentScene->PropagateEnterTree(this);
    }
}

void Core::Notification(Node3d *node, const NodeNotification notification) {
    switch (notification) {
        case NodeNotification::EnterTree: {
            m_nodeCache.push_back(node);
            if (auto* r = dynamic_cast<RenderableNode3d*>(node)) {
                m_renderableCache.push_back(r);
            }

            if (auto* l = dynamic_cast<LightNode3d*>(node)) {
                m_lightNodeCache.push_back(l);
                RegisterLight(l);
            }

            break;
        }

        case NodeNotification::ExitTree: {
            std::erase(m_nodeCache, node);
            if (auto* r = dynamic_cast<RenderableNode3d*>(node)) {
                std::erase(m_renderableCache, r);
            }

            if (auto* l = dynamic_cast<LightNode3d*>(node)) {
                std::erase(m_lightNodeCache, l);
                UnregisterLight(l);
            }

            break;
        }

        case NodeNotification::Ready: {
            // nothing needed here yet
            break;
        }
    }
}

GLuint Core::GetMainViewportTexture() const {
    return m_mainFramebuffer ? m_mainFramebuffer->GetColorAttachment() : 0;
}

void Core::InitRenderer() {
    RenderApi::InitSDL();

    m_renderer = std::make_unique<RenderApi>();
    m_activeWindow = std::move(m_renderer->CreateWindow("Mango", {1280, 720}, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL));

    int w, h;
    SDL_GetWindowSizeInPixels(m_activeWindow->GetSDLWindow(), &w, &h);
    m_mainFramebuffer = std::make_unique<Framebuffer>(w, h, FramebufferType::ColorDepth);

    std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "Version: " << glGetString(GL_VERSION) << std::endl;

    SDL_GL_SetSwapInterval(0);
    SDL_SetWindowRelativeMouseMode(m_activeWindow->GetSDLWindow(), true);

    m_defaultShader = ResourceManager::Get().LoadShader("default", "../Assets/Shaders/test.vert", "../Assets/Shaders/test.frag");
}

void Core::InitImGui() const {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    EditorStyle::Get().Init(io);

    ImGui_ImplSDL3_InitForOpenGL(m_activeWindow->GetSDLWindow(), m_activeWindow->GetContext());
    ImGui_ImplOpenGL3_Init("#version 460");
}

void Core::BeginImGuiFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    constexpr ImGuiWindowFlags host_flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDocking;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("##DockspaceHost", nullptr, host_flags);
    ImGui::PopStyleVar(3);

    ImGui::DockSpace(ImGui::GetID("MainDockspace"), ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);

    ImGui::End();
}

void Core::EndImGuiFrame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Core::SwapBuffers() const {
    m_activeWindow->SwapBuffers();
}

bool Core::PollEvents() const {
    Input::BeginFrame();

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);
        Input::ProcessEvent(event);

        if (event.type == SDL_EVENT_WINDOW_RESIZED) {
            int w, h;
            SDL_GetWindowSizeInPixels(m_activeWindow->GetSDLWindow(), &w, &h);
            if (w > 0 && h > 0) {
                m_mainFramebuffer->Resize(w, h);
            }
        }

        if (event.type == SDL_EVENT_QUIT) {
            m_activeWindow->Close();
        }

        if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            if (event.window.windowID == SDL_GetWindowID(m_activeWindow->GetSDLWindow())) {
                m_activeWindow->Close();
            }
        }
    }

    Input::EndFrame();
    return m_activeWindow->IsOpen();
}

bool Core::IsInScene(const Node3d* node, const Node3d* sceneRoot) {
    while (node) {
        if (node == sceneRoot) return true;
        node = node->GetParent();
    }

    return false;
}

void Core::RenderScene(Node3d* sceneRoot, const CameraNode3d* camera, const Framebuffer* targetFbo) const {
    if (!camera || !targetFbo || !sceneRoot) {
        return;
    }

    sceneRoot->UpdateWorldTransform();
    m_renderer->ClearQueues();

    if (m_globalSkybox) {
        m_renderer->SetSkybox(m_globalSkybox.get());
    }

    for (auto* l : m_lightNodeCache) {
        if (IsInScene(l, sceneRoot)) {
            l->SyncLight();
        }
    }

    for (auto* renderable : m_renderableCache) {
        if (!IsInScene(renderable, sceneRoot) || !renderable->IsVisible()) {
            continue;
        }

        if (auto* portal = dynamic_cast<PortalNode3d*>(renderable)) {
            if (portal->GetMesh() && portal->GetShader()) {
                m_renderer->SubmitPortal(portal);
            }
            continue;
        }

        if (auto* meshNode = dynamic_cast<MeshNode3d*>(renderable)) {
            if (meshNode->GetMesh() && meshNode->GetShader()) {
                m_renderer->SubmitMesh(meshNode);
            }
        }
    }

    m_renderer->RenderSceneWithPortals(camera, targetFbo, 6);

    if (m_cameraMode == CameraMode::Editor) {
        m_renderer->DrawGrid(camera, targetFbo);
    }
}

void Core::StepFrame(const float deltaTime) {
    m_accumulator += deltaTime;
    while (m_accumulator >= FIXED_TIMESTEP) {
        for (auto* node : m_nodeCache) {
            node->PhysicsProcess(FIXED_TIMESTEP);
        }
        m_accumulator -= FIXED_TIMESTEP;
    }

    for (auto* node : m_nodeCache) {
        node->Process(deltaTime);
    }

    for (auto* node : m_nodeCache) {
        if (!node->GetParent()) {
            node->UpdateWorldTransform();
        }
    }
}

void Core::Process() {
    uint64_t lastTime = SDL_GetTicksNS();

    while (m_activeWindow->IsOpen()) {
        const uint64_t now = SDL_GetTicksNS();
        const float deltaTime = (now - lastTime) / 1e9f;
        lastTime = now;

        PollEvents();
        StepFrame(deltaTime);

        if (m_activeCamera) {
            RenderScene(m_currentScene.get(), m_activeCamera, m_mainFramebuffer.get());
        }

        BeginImGuiFrame();
        EndImGuiFrame();

        SwapBuffers();
    }
}

void Core::RegisterScene(Node3d *scene) {
    scene->PropagateEnterTree(this);
}

void Core::UnregisterScene(Node3d *scene) {
    scene->PropagateExitTree();
}

bool Core::IsNodeCached(const Node3d* node) const {
    return std::ranges::find(m_nodeCache, node) != m_nodeCache.end();
}

void Core::ApplyCameraMode() {
    CameraNode3d* desired = nullptr;

    if (m_cameraMode == CameraMode::Editor) {
        desired = m_editorCamera;
    } else {
        desired = m_gameCamera;
    }

    if (!desired) {
        desired = m_cameraMode == CameraMode::Editor ? m_gameCamera : m_editorCamera;
    }

    SetActiveCamera(desired);
}

void Core::RegisterLight(LightNode3d *light) const {
    if (auto* d = dynamic_cast<DirectionalLightNode3d*>(light)) {
        m_renderer->AddDirectionalLight(d->GetLight());
    } else if (auto* p = dynamic_cast<PointLightNode3d*>(light)) {
        m_renderer->AddPointLight(p->GetLight());
    } else if (auto* s = dynamic_cast<SpotLightNode3d*>(light)) {
        m_renderer->AddSpotLight(s->GetLight());
    }
}

void Core::UnregisterLight(LightNode3d *light) const {
    if (auto* d = dynamic_cast<DirectionalLightNode3d*>(light)) {
        m_renderer->RemoveDirectionalLight(d->GetLight());
    } else if (auto* p = dynamic_cast<PointLightNode3d*>(light)) {
        m_renderer->RemovePointLight(p->GetLight());
    } else if (auto* s = dynamic_cast<SpotLightNode3d*>(light)) {
        m_renderer->RemoveSpotLight(s->GetLight());
    }
}

void Core::RebuildNodeCache() {
    std::vector<Node3d*> roots;
    for (auto* node : m_nodeCache) {
        if (!node->GetParent()) {
            roots.push_back(node);
        }
    }

    m_nodeCache.clear();
    m_renderableCache.clear();
    m_lightNodeCache.clear();

    for (auto* root : roots) {
        root->PropagateEnterTree(this);
    }
}

void Core::SetEditorCamera(CameraNode3d *camera) {
    m_editorCamera = camera;
    if (m_cameraMode == CameraMode::Editor || m_activeCamera == nullptr) {
        ApplyCameraMode();
    }
}

void Core::SetGameCamera(CameraNode3d *camera) {
    m_gameCamera = camera;
    if (m_cameraMode == CameraMode::Game) {
        ApplyCameraMode();
    }
}

void Core::SetActiveCamera(CameraNode3d* camera) {
    m_activeCamera = camera;
}

void Core::SetGlobalSkybox(std::unique_ptr<SkyboxNode3d> skybox) {
    if (m_globalSkybox) {
        m_globalSkybox->PropagateExitTree();
    }

    m_globalSkybox = std::move(skybox);

    if (m_globalSkybox) {
        m_globalSkybox->PropagateEnterTree(this);
    }
}

void Core::SetCameraMode(const CameraMode mode) {
    m_cameraMode = mode;
    ApplyCameraMode();
}

void Core::ChangeScene(std::unique_ptr<Node3d> scene) {
    if (m_currentScene) {
        m_currentScene->PropagateExitTree();
    }

    m_currentScene.reset();
    m_currentScene = std::move(scene);

    m_gameCamera = nullptr;

    if (m_currentScene) {
        m_currentScene->PropagateEnterTree(this);
    }
}
