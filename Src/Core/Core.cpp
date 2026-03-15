
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

Core::Core(Node3d* scene) : m_currentScene(scene) {}

Core::~Core() {
    // renderer must be destroyed before ImGui shutdown since it owns GL resources
    m_renderer.reset();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    delete m_currentScene;
}

void Core::Init() {
    InitRenderer();
    InitImGui();

    Input::SetMouseDeltaEnabled(m_mouseCaptured);

    m_currentScene->PropagateEnterTree(this);
}

void Core::Notification(Node3d *node, const NodeNotification notification) {
    switch (notification) {
        case NodeNotification::EnterTree: {
            m_nodeCache.push_back(node);
            if (auto* r = dynamic_cast<RenderableNode3d*>(node)) {
                m_renderableCache.push_back(r);
            }

            if (auto* l = dynamic_cast<LightNode3d*>(node)) {
                RegisterLight(l);
            }

            if (auto* s = dynamic_cast<SkyboxNode3d*>(node)) {
                m_activeSkybox = s;
                m_renderer->SetSkybox(s);
            }

            break;
        }

        case NodeNotification::ExitTree: {
            std::erase(m_nodeCache, node);
            if (auto* r = dynamic_cast<RenderableNode3d*>(node)) {
                std::erase(m_renderableCache, r);
            }

            if (auto* l = dynamic_cast<LightNode3d*>(node)) {
                UnregisterLight(l);
            }

            if (auto* s = dynamic_cast<SkyboxNode3d*>(node)) {
                m_activeSkybox = nullptr;
                m_renderer->SetSkybox(nullptr);
            }

            break;
        }

        case NodeNotification::Ready: {
            // nothing needed here yet
            break;
        }
    }
}

void Core::InitRenderer() {
    RenderApi::InitSDL();

    m_renderer     = std::make_unique<RenderApi>();
    m_activeWindow = m_renderer->CreateWindow("Mango", {500, 500}, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);

    std::cout << "Vendor: "   << glGetString(GL_VENDOR)   << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER)  << std::endl;
    std::cout << "Version: "  << glGetString(GL_VERSION)   << std::endl;

    SDL_GL_SetSwapInterval(0);
    SDL_SetWindowRelativeMouseMode(m_activeWindow->GetSDLWindow(), true);
}

void Core::InitImGui() const {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplSDL3_InitForOpenGL(m_activeWindow->GetSDLWindow(), m_activeWindow->GetContext());
    ImGui_ImplOpenGL3_Init("#version 460");
}

void Core::BeginImGuiFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void Core::EndImGuiFrame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Core::BuildNodeCache(Node3d *node) {
    m_nodeCache.push_back(node);
    if (auto* renderable = dynamic_cast<RenderableNode3d*>(node)) {
        m_renderableCache.push_back(renderable);
    }

    for (const auto child : node->GetChildren()) {
        BuildNodeCache(child);
    }
}

bool Core::IsNodeCached(const Node3d* node) const {
    return std::ranges::find(m_nodeCache, node) != m_nodeCache.end();
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
    m_nodeCache.clear();
    m_renderableCache.clear();

    BuildNodeCache(m_currentScene);
}

void Core::SetActiveCamera(CameraNode3d *camera) {
    m_activeCamera = camera;
    m_renderer->SetActiveCamera(camera);
}

void Core::Process() {
    constexpr float FIXED_TIMESTEP = 1.0f / 60.0f;
    float accumulator = 0.0f;
    uint64_t lastTime = SDL_GetTicksNS();

    float cpuMs = 0.0f;
    float smoothCpuMs = 0.0f;

    while (m_activeWindow->IsOpen()) {
        const uint64_t cpuFrameStart = SDL_GetTicksNS();
        const uint64_t now = SDL_GetTicksNS();
        const float deltaTime = (now - lastTime) / 1e9f;
        lastTime = now;

        Input::BeginFrame();

        // TODO: add an event handler class / manager
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            Input::ProcessEvent(event);
            m_renderer->HandleResizeEvent(event);

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

        if (Input::IsKeyJustPressed(SDL_SCANCODE_TAB)) {
            m_mouseCaptured = !m_mouseCaptured;
            SDL_SetWindowRelativeMouseMode(m_activeWindow->GetSDLWindow(), m_mouseCaptured);
            Input::SetMouseDeltaEnabled(m_mouseCaptured);
        }

        if (Input::IsKeyJustPressed(SDL_SCANCODE_ESCAPE)) {
            m_activeWindow->Close();
        }

        accumulator += deltaTime;
        while (accumulator >= FIXED_TIMESTEP) {
            for (auto* node : m_nodeCache) {
                node->PhysicsProcess(FIXED_TIMESTEP);
            }

            accumulator -= FIXED_TIMESTEP;
        }

        for (auto* node : m_nodeCache) {
            node->Process(deltaTime);
        }
        m_currentScene->UpdateWorldTransform();

        BeginImGuiFrame();

        const uint64_t cpuFrameEnd = SDL_GetTicksNS();
        cpuMs = static_cast<float>(cpuFrameEnd - cpuFrameStart) / 1'000'000.0f;
        smoothCpuMs = smoothCpuMs * 0.95f + cpuMs * 0.05f;

        ImGui::Begin("Stats");
        ImGui::Text("FPS:      %.1f", ImGui::GetIO().Framerate);
        ImGui::Text("GPU Time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);
        ImGui::Text("CPU Time: %.3f ms", smoothCpuMs);
        ImGui::End();

        m_renderer->ClearColour({0.16f, 0.16f, 0.16f, 1.0f});
        for (auto* renderable : m_renderableCache) {
            renderable->SubmitToRenderer(*m_renderer);
        }
        m_renderer->Flush();

        EndImGuiFrame();

        m_activeWindow->SwapBuffers();
    }
}

void Core::ChangeScene(Node3d* scene) {
    delete m_currentScene;
    m_currentScene = scene;
    m_nodeCache.clear();
    m_renderableCache.clear();
    m_currentScene->PropagateEnterTree(this);
}
