#include <imgui_impl_sdl3.h>
#include <iostream>
#include <random>

#include "Core/RenderApi.h"
#include <glm/glm.hpp>

#include "Scene/Camera.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "Core/Window.h"
#include "glad/gl.h"
#include "glm/gtc/type_ptr.hpp"
#include "Renderer/GltfLoader.h"

int main() {
    RenderApi::Init();
    Window* window = RenderApi::CreateWindow("Mango", {500, 500}, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);

    std::cout << "Vendor: "   << glGetString(GL_VENDOR)   << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER)  << std::endl;
    std::cout << "Version: "  << glGetString(GL_VERSION)   << std::endl;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplSDL3_InitForOpenGL(window->GetSDLWindow(), window->GetContext());
    ImGui_ImplOpenGL3_Init("#version 460");

    SDL_GL_SetSwapInterval(0);

    bool mouseCaptured = true;
    SDL_SetWindowRelativeMouseMode(window->GetSDLWindow(), mouseCaptured);

    Mesh* mesh = new Mesh(
        {
            // Back face (normal points -Z)
            {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
            {{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
            {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
            {{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},

            // Front face (normal points +Z)
            {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
            {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
            {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
            {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},

            // Left face (normal points -X)
            {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
            {{-0.5f, -0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
            {{-0.5f,  0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
            {{-0.5f,  0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},

            // Right face (normal points +X)
            {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
            {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
            {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
            {{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},

            // Bottom face (normal points -Y)
            {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
            {{ 0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
            {{ 0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
            {{-0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},

            // Top face (normal points +Y)
            {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
            {{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
            {{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
            {{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        },{
            // Back face
            1,  0,  2,  2,  0,  3,
            // Front face
            4,  5,  6,  6,  7,  4,
            // Left face
            8,  9,  10, 10, 11, 8,
            // Right face
            13, 12, 14, 14, 12, 15,
            // Bottom face
            16, 17, 18, 18, 19, 16,
            // Top face
            21, 20, 22, 22, 20, 23
        }
    );

    std::vector<Object*> stressObjects;
    std::vector<PointLight*> stressLights;

    auto texture = std::make_shared<Texture>("../Assets/Textures/face.png");
    Shader* shader = new Shader("../Assets/Shaders/test.vert", "../Assets/Shaders/test.frag");

    // const Shader* shader2 = new Shader("../Assets/Shaders/new_test.vert", "../Assets/Shaders/new_test.frag");

    const auto gltfObjects = GltfLoader::Load("../Assets/Models/teddy.glb", shader);

    Object* object = new Object(mesh, shader);
    object->transform.Position = {0, -5, 0};
    object->transform.Scale = {100, 0.5, 100};
    object->GetMaterial().SetDiffuse(texture);
    stressObjects.push_back(object);

    Object* object1 = new Object(mesh, shader);
    object1->transform.Position = {0, 3, -3};
    object1->GetMaterial().SetDiffuse(texture);
    stressObjects.push_back(object1);

    Object* object2 = new Object(mesh, shader);
    object2->transform.Position = {-1, 2, 2};
    object2->GetMaterial().SetDiffuse(texture);
    stressObjects.push_back(object2);

    Object* object3 = new Object(mesh, shader);
    object3->transform.Position = {3, 1, 1};
    object3->GetMaterial().SetDiffuse(texture);
    stressObjects.push_back(object3);

    DirectionalLight* directionalLight = new DirectionalLight({0.5f, -1.0f, 0.5f}, {1.0f, 1.0f, 1.0f}, 0.05f);
    RenderApi::AddDirectionalLight(directionalLight);

    PointLight* pointLight = new PointLight({0, 1, 0}, {1.0, 0.2, 0.1}, 1.5f);
    pointLight->SetAttenuation(1.0, 0.22, 0.20);
    RenderApi::AddPointLight(pointLight);
    stressLights.push_back(pointLight);

    // stress test
    std::mt19937 rng(42);
    auto randFloat = [&](float min, float max) {
        return std::uniform_real_distribution<float>(min, max)(rng);
    };

    constexpr int NUM_OBJECTS = 1000;
    constexpr int NUM_LIGHTS  = 30;

    for (int i = 0; i < NUM_OBJECTS; i++) {
        Object* obj = new Object(mesh, shader);
        obj->transform.Position = { randFloat(-30, 30), randFloat(-5, 10), randFloat(-30, 30) };
        obj->transform.Scale    = { randFloat(0.3f, 2.0f), randFloat(0.3f, 2.0f), randFloat(0.3f, 2.0f) };
        obj->GetMaterial().SetDiffuse(texture);
        stressObjects.push_back(obj);
    }

    for (int i = 0; i < NUM_LIGHTS; i++) {
        PointLight* light = new PointLight(
            { randFloat(-30, 30), randFloat(0, 8), randFloat(-30, 30) },
            // { 0, 1, 0 },
            { randFloat(0.1f, 1.0f), randFloat(0.1f, 1.0f), randFloat(0.1, 1.0f) },
            randFloat(0.1f, 1.0f)
        );
        light->SetAttenuation(1.0, 0.22, 0.20);

        RenderApi::AddPointLight(light);
        stressLights.push_back(light);
    }

    Camera* camera = new Camera({0, 0, 3}, 75.0f, static_cast<float>(window->GetSize().x) / static_cast<float>(window->GetSize().y), 0.1f, 100.0f);
    RenderApi::SetActiveCamera(camera);

    RenderApi::RebuildClusters();

    uint32_t lastTime = SDL_GetTicks();
    SDL_Event event;
    while (window->IsOpen()) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Renderer");

        // frame time graph
        static float frameTimes[128] = {};
        static int frameIndex = 0;
        const float ms = 1000.0f / ImGui::GetIO().Framerate;
        frameTimes[frameIndex] = ms;
        frameIndex = (frameIndex + 1) % 128;
        char overlay[32];
        snprintf(overlay, sizeof(overlay), "%.2f ms", ms);
        ImGui::PlotLines("##frametime", frameTimes, 128, frameIndex, overlay, 0.0f, 33.0f, ImVec2(0, 60));

        ImGui::Separator();

        ImGui::Text("FPS:          %.1f", ImGui::GetIO().Framerate);
        ImGui::Text("Frame Time:   %.3f ms", ms);

        ImGui::Separator();

        ImGui::Text("Submitted:    %u", RenderApi::GetSubmittedCount());
        ImGui::Text("Draw Calls:   %u", RenderApi::GetDrawCallCount());
        ImGui::Text("Shadow DCs:   %u", RenderApi::GetShadowDrawCallCount());
        ImGui::Text("Triangles:    %u", RenderApi::GetTriangleCount());

        ImGui::Separator();
        const uint32_t submitted = RenderApi::GetSubmittedCount();
        const uint32_t culled    = RenderApi::GetCulledCount();
        const uint32_t drawn     = submitted - culled;
        const float cullPct      = submitted > 0 ? (culled / static_cast<float>(submitted)) * 100.0f : 0.0f;

        ImGui::Text("Culled:       %u / %u (%.0f%%)", culled, submitted, cullPct);

        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.8f, 0.3f, 1.0f));
        ImGui::ProgressBar(culled / static_cast<float>(std::max(submitted, 1u)), ImVec2(-1, 0), "Cull Efficiency");
        ImGui::PopStyleColor();

        ImGui::Separator();

        ImGui::Text("Point Lights: %u", NUM_LIGHTS + 1);
        ImGui::Text("Spot Lights:  0");
        ImGui::Text("Dir Lights:   %u", 1);

        ImGui::Separator();

        // debug modes
        static int debugMode = 0;
        const char* debugModes[] = { "None", "Normal", "Heatmap", "Z-Slices", "XY-Tiles", "Shadow Map" };
        if (ImGui::Combo("Debug Mode", &debugMode, debugModes, 6)) {
            shader->SetInt("u_DebugMode", debugMode);
        }

        static bool showClusterBounds = false;
        ImGui::Checkbox("Show Cluster Bounds", &showClusterBounds);

        static bool showShadowMap = false;
        ImGui::Checkbox("Show Shadow Map", &showShadowMap);
        if (showShadowMap && !RenderApi::GetShadowMaps().empty()) {
            const ImTextureID texId = static_cast<uint64_t>(RenderApi::GetShadowMaps()[0]->GetDepthTexture());
            ImGui::Image(texId, ImVec2(256, 256));
        }

        ImGui::End();

        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);

            if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) {
                window->Close();
            }

            if (event.type == SDL_EVENT_MOUSE_MOTION && mouseCaptured) {
                camera->Rotate(event.motion.xrel * 0.05f, -event.motion.yrel * 0.05f);
            }

            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_TAB) {
                mouseCaptured = !mouseCaptured;
                SDL_SetWindowRelativeMouseMode(window->GetSDLWindow(), mouseCaptured);
            }

            RenderApi::HandleResizeEvent(event);
        }

        const uint64_t currentTime = SDL_GetTicks();
        const float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        const bool* keys = SDL_GetKeyboardState(nullptr);
        float speed = 5.0f;
        if (keys[SDL_SCANCODE_LSHIFT]) speed += 50.0f;
        if (keys[SDL_SCANCODE_W]) camera->Move(camera->GetFront() * speed * deltaTime);
        if (keys[SDL_SCANCODE_S]) camera->Move(-camera->GetFront() * speed * deltaTime);
        if (keys[SDL_SCANCODE_A]) camera->Move(-camera->GetRight() * speed * deltaTime);
        if (keys[SDL_SCANCODE_D]) camera->Move(camera->GetRight() * speed * deltaTime);

        window->MakeCurrent();

        // depth pass
        RenderApi::Submit(object);
        RenderApi::Submit(object1);
        RenderApi::Submit(object2);
        RenderApi::Submit(object3);

        for (auto* obj : stressObjects) RenderApi::Submit(obj);
        for (auto* coolobj : gltfObjects) RenderApi::Submit(coolobj);

        RenderApi::ClearColour({0.12f, 0.12f, 0.12f, 1.0f});
        RenderApi::Flush();

        // shows the clustered forward rendering clusters, shows size and z index
        if (showClusterBounds) {
            RenderApi::DrawClusterVisualizer();
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        window->SwapBuffers();
    }

    // SDL_GL_DeleteContext(glContext);

    delete mesh;
    delete directionalLight;
    delete camera;
    delete shader;

    for (auto* obj : stressObjects) delete obj;
    for (auto* light : stressLights) {
        RenderApi::RemovePointLight(light);
        delete light;
    }

    for (auto* coolobj : gltfObjects) delete coolobj;

    texture.reset();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_Quit();
    return 0;
}
