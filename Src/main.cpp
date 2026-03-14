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

static void ShowPointShadowDebugUI(const RenderApi& renderer);

int main() {
    RenderApi::InitSDL();
    RenderApi renderer;

    Window* window = renderer.CreateWindow("Mango", {500, 500}, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);

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

    const auto gltfObjects = GltfLoader::Load("../Assets/Models/teddy.glb", shader);

    Object* object = new Object(mesh, shader);
    object->transform.Position = {0, -5, 0};
    object->transform.Scale = {100, 0.5, 100};
    object->GetMaterial().SetAlbedoColor(glm::vec4(1.0, 1.0, 1.0, 1.0));
    stressObjects.push_back(object);

    Object* object1 = new Object(mesh, shader);
    object1->transform.Position = {0, -4.5, 0};
    object1->transform.Scale = {1.0, 2.0, 100};
    object1->GetMaterial().SetDiffuse(texture);
    stressObjects.push_back(object1);

    Object* object2 = new Object(mesh, shader);
    object2->transform.Position = {0, -2.5, 2};
    object2->GetMaterial().SetDiffuse(texture);
    stressObjects.push_back(object2);

    Object* object3 = new Object(mesh, shader);
    object3->transform.Position = {0, -2.5, -2};
    object3->GetMaterial().SetDiffuse(texture);
    stressObjects.push_back(object3);

    DirectionalLight* directionalLight = new DirectionalLight({0.5f, -0.6f, -0.5f}, {0.9f, 0.65f, 0.32f}, 0.1f);
    renderer.AddDirectionalLight(directionalLight);

    PointLight* pointLight = new PointLight({-5, 2, 0}, {1.0, 0.2, 0.1}, 1.5f, 15.0f);
    pointLight->SetAttenuation(1.0, 0.22, 0.20);
    renderer.AddPointLight(pointLight);
    stressLights.push_back(pointLight);

    // stress test
    std::mt19937 rng(42);
    auto randFloat = [&](const float min, const float max) {
        return std::uniform_real_distribution(min, max)(rng);
    };

    constexpr int NUM_OBJECTS = 50;
    constexpr int NUM_LIGHTS  = 10;

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
            randFloat(0.1f, 1.0f),
            randFloat(8.0f, 20.0f)
        );
        light->SetAttenuation(1.0, 0.22, 0.20);

        renderer.AddPointLight(light);
        stressLights.push_back(light);
    }

    std::vector<std::string> skyboxTextures = {
        "../Assets/Textures/Cubemaps/sky_16_2k/sky_16_cubemap_2k/px.png", // +X (right)
        "../Assets/Textures/Cubemaps/sky_16_2k/sky_16_cubemap_2k/nx.png", // -X (left)
        "../Assets/Textures/Cubemaps/sky_16_2k/sky_16_cubemap_2k/py.png", // +Y (top)
        "../Assets/Textures/Cubemaps/sky_16_2k/sky_16_cubemap_2k/ny.png", // -Y (bottom)
        "../Assets/Textures/Cubemaps/sky_16_2k/sky_16_cubemap_2k/pz.png", // +Z (front)
        "../Assets/Textures/Cubemaps/sky_16_2k/sky_16_cubemap_2k/nz.png", // -Z (back)
    };
    Skybox* skybox = new Skybox(skyboxTextures);
    renderer.SetSkybox(skybox);

    Camera* camera = new Camera({0, 0, 3}, 75.0f, static_cast<float>(window->GetSize().x) / static_cast<float>(window->GetSize().y), 0.1f, 200.0f);
    renderer.SetActiveCamera(camera);

    uint32_t lastTime = SDL_GetTicks();

    float cpuMs = 0.0f;
    static float cpuTimes[128] = {};
    static int cpuTimeIndex = 0;

    SDL_Event event;
    while (window->IsOpen()) {
        const uint64_t cpuFrameStart = SDL_GetTicksNS();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        ShowPointShadowDebugUI(renderer);

        ImGui::Begin("Renderer");

        // frame time graph
        static float frameTimes[128] = {};
        static int frameIndex = 0;
        const float ms = 1000.0f / ImGui::GetIO().Framerate;
        frameTimes[frameIndex] = ms;
        frameIndex = (frameIndex + 1) % 128;
        char overlay[32];
        snprintf(overlay, sizeof(overlay), "%.2f ms", ms);
        ImGui::PlotLines("##frametime", frameTimes, 128, frameIndex, overlay, 0.0f, 33.0f, ImVec2(0, 40));
        ImGui::SameLine();
        ImGui::Text("GPU");

        // CPU time graph
        cpuTimes[cpuTimeIndex] = cpuMs;
        cpuTimeIndex = (cpuTimeIndex + 1) % 128;
        char cpuOverlay[32];
        snprintf(cpuOverlay, sizeof(cpuOverlay), "%.2f ms", cpuMs);
        ImGui::PlotLines("##cputime", cpuTimes, 128, cpuTimeIndex, cpuOverlay, 0.0f, 33.0f, ImVec2(0, 40));
        ImGui::SameLine();
        ImGui::Text("CPU");

        ImGui::Separator();

        ImGui::Text("FPS:          %.1f", ImGui::GetIO().Framerate);
        ImGui::Text("GPI Time:     %.3f ms", ms);
        ImGui::Text("CPU Time:     %.3f ms", cpuMs);

        ImGui::Separator();

        ImGui::Text("Submitted:    %u", renderer.GetStats().submitted);
        ImGui::Text("Draw Calls:   %u", renderer.GetStats().drawCalls);
        ImGui::Text("Shadow DCs:   %u", renderer.GetStats().shadowDrawCalls);
        ImGui::Text("Triangles:    %u", renderer.GetStats().triangles);

        ImGui::Separator();

        const uint32_t submitted = renderer.GetStats().submitted;
        const uint32_t culled    = renderer.GetStats().culled;
        const float cullPct      = submitted > 0 ? culled / static_cast<float>(submitted) * 100.0f : 0.0f;

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
        static int debugCascade = 0;

        const char* debugModes[] = { "CF None", "CF Normal", "CF Heatmap", "CF Z-Slices", "CF XY-Tiles", "CSM", "CSM-Slices", "CSM Proj Coords", "CSM Shadow Factor" };
        if (ImGui::Combo("Debug Mode", &debugMode, debugModes, IM_ARRAYSIZE(debugModes))) {
            renderer.SetDebugMode(debugMode);
        }

        if (debugMode == 5) {
            ImGui::SliderInt("Cascade", &debugCascade, 0, CascadedShadowMap::NUM_CASCADES - 1);
            renderer.SetDebugCascade(debugCascade);
        }

        static bool showClusterBounds = false;
        ImGui::Checkbox("Show Cluster Bounds", &showClusterBounds);

        ImGui::End();

        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);

            if (event.type == SDL_EVENT_QUIT || (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)) {
                window->Close();
            }

            if (event.type == SDL_EVENT_MOUSE_MOTION && mouseCaptured) {
                camera->Rotate(event.motion.xrel * 0.05f, -event.motion.yrel * 0.05f);
            }

            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_TAB) {
                mouseCaptured = !mouseCaptured;
                SDL_SetWindowRelativeMouseMode(window->GetSDLWindow(), mouseCaptured);
            }

            renderer.HandleResizeEvent(event);
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

        for (auto* obj : stressObjects) renderer.Submit(obj);
        for (auto* coolobj : gltfObjects) renderer.Submit(coolobj);

        renderer.ClearColour({0.16f, 0.16f, 0.16f, 1.0f});
        renderer.Flush();

        if (showClusterBounds) {
            renderer.DrawClusterVisualizer();
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        const uint64_t cpuFrameEnd = SDL_GetTicksNS();
        cpuMs = static_cast<float>(cpuFrameEnd - cpuFrameStart) / 1'000'000.0f;

        window->SwapBuffers();
    }

    // SDL_GL_DeleteContext(glContext);

    delete mesh;
    delete directionalLight;
    delete skybox;
    delete camera;
    delete shader;

    for (auto* obj : stressObjects)  delete obj;
    for (auto* obj : gltfObjects)    delete obj;

    for (auto* light : stressLights) {
        renderer.RemovePointLight(light);
        delete light;
    }

    texture.reset();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    return 0;
}

static void ShowPointShadowDebugUI(const RenderApi& renderer) {
    if (!ImGui::Begin("Point Light Shadows")) {
        ImGui::End();
        return;
    }

    const uint32_t totalPoint = renderer.GetPointLightCount();
    const uint32_t maxShadow  = renderer.GetMaxShadowedPointLights();
    const uint32_t shadowed   = renderer.GetShadowedPointLightCount();

    ImGui::Text("Total point lights:        %u", totalPoint);
    ImGui::Text("Max shadowed point lights: %u", maxShadow);
    ImGui::Text("Shadowed this frame:       %u", shadowed);
    ImGui::Text("Shadow faces rendered:     %u", shadowed * 6u);

    ImGui::Separator();

    const auto& dbg = renderer.GetShadowedPointLightsDebug();
    if (ImGui::BeginTable("shadowed_point_lights", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, 240))) {
        ImGui::TableSetupColumn("Slot");
        ImGui::TableSetupColumn("LightIdx");
        ImGui::TableSetupColumn("Score");
        ImGui::TableSetupColumn("Radius");
        ImGui::TableSetupColumn("FarPlane");
        ImGui::TableSetupColumn("DistToCam");
        ImGui::TableSetupColumn("Pos");
        ImGui::TableHeadersRow();

        for (const auto& l : dbg) {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0); ImGui::Text("%u", l.slot);
            ImGui::TableSetColumnIndex(1); ImGui::Text("%u", l.lightIndex);
            ImGui::TableSetColumnIndex(2); ImGui::Text("%.3f", l.score);
            ImGui::TableSetColumnIndex(3); ImGui::Text("%.2f", l.radius);
            ImGui::TableSetColumnIndex(4); ImGui::Text("%.2f", l.farPlane);
            ImGui::TableSetColumnIndex(5); ImGui::Text("%.2f", l.distanceToCamera);
            ImGui::TableSetColumnIndex(6); ImGui::Text("(%.1f, %.1f, %.1f)", l.position.x, l.position.y, l.position.z);
        }

        ImGui::EndTable();
    }

    ImGui::End();
}
