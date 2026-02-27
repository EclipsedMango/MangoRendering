#include <iostream>

#include "RenderApi.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Camera.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"
#include "glad/gl.h"
#include "glm/gtc/type_ptr.hpp"
#include "SDL2/SDL.h"

int main() {
    RenderApi::Init();
    Window* window = RenderApi::CreateWindow("Mango", {SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED}, {500, 500}, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);

    std::cout << "Vendor: "   << glGetString(GL_VENDOR)   << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER)  << std::endl;
    std::cout << "Version: "  << glGetString(GL_VERSION)   << std::endl;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForOpenGL(window->GetSDLWindow(), window->GetContext());
    ImGui_ImplOpenGL3_Init("#version 460");

    SDL_GL_SetSwapInterval(0);

    bool mouseCaptured = true;
    SDL_SetRelativeMouseMode(static_cast<SDL_bool>(mouseCaptured));

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
        },
        {
            // Back face
            0,  1,  2,  2,  3,  0,
            // Front face
            4,  5,  6,  6,  7,  4,
            // Left face
            8,  9,  10, 10, 11, 8,
            // Right face
            12, 13, 14, 14, 15, 12,
            // Bottom face
            16, 17, 18, 18, 19, 16,
            // Top face
            20, 21, 22, 22, 23, 20
        }
    );

    Texture* texture = new Texture("Assets/Textures/face.png");
    Shader* shader = new Shader("Shaders/test.vert", "Shaders/test.frag");

    Object* object = new Object(mesh, shader);
    object->transform.Scale = {10, 0.5, 10};

    Object* object1 = new Object(mesh, shader);
    object1->transform.Position = {0, 3, -3};

    Object* object2 = new Object(mesh, shader);
    object2->transform.Position = {-1, 2, 2};

    Object* object3 = new Object(mesh, shader);
    object3->transform.Position = {3, 1, 1};

    object->AddTexture(texture);
    object1->AddTexture(texture);
    object2->AddTexture(texture);
    object3->AddTexture(texture);

    Camera* camera = new Camera({0, 0, 3}, 75.0f, static_cast<float>(window->GetSize().x) / static_cast<float>(window->GetSize().y), 0.1f, 100.0f);
    RenderApi::SetActiveCamera(camera);

    DirectionalLight* directionalLight = new DirectionalLight({0.5f, -1.0f, 0.5f}, {1.0f, 1.0f, 1.0f}, 1.0f);
    RenderApi::AddDirectionalLight(directionalLight);

    PointLight* pointLight = new PointLight({0, 1, 0}, {1.0, 0.2, 0.1}, 1.5f);
    RenderApi::AddPointLight(pointLight);

    uint32_t lastTime = SDL_GetTicks();
    SDL_Event event;
    while (window->IsOpen()) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Info");
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::End();

        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);

            if (event.type == SDL_QUIT || event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                window->Close();
            }

            if (event.type == SDL_MOUSEMOTION && mouseCaptured) {
                camera->Rotate(event.motion.xrel * 0.05f, -event.motion.yrel * 0.05f);
            }

            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_TAB) {
                mouseCaptured = !mouseCaptured;
                SDL_SetRelativeMouseMode(static_cast<SDL_bool>(mouseCaptured));
            }

            RenderApi::HandleResizeEvent(event);
        }

        const uint32_t currentTime = SDL_GetTicks();
        const float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        const uint8_t* keys = SDL_GetKeyboardState(nullptr);
        constexpr float speed = 5.0f;
        if (keys[SDL_SCANCODE_W]) camera->Move(camera->GetFront() * speed * deltaTime);
        if (keys[SDL_SCANCODE_S]) camera->Move(-camera->GetFront() * speed * deltaTime);
        if (keys[SDL_SCANCODE_A]) camera->Move(-camera->GetRight() * speed * deltaTime);
        if (keys[SDL_SCANCODE_D]) camera->Move(camera->GetRight() * speed * deltaTime);

        window->MakeCurrent();
        RenderApi::ClearColour({0.12f, 0.12f, 0.12f, 1.0f});
        RenderApi::UploadCameraData();
        RenderApi::UploadLightData();

        RenderApi::DrawObject(object);
        RenderApi::DrawObject(object1);
        RenderApi::DrawObject(object2);
        RenderApi::DrawObject(object3);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        window->SwapBuffers();
    }

    // SDL_GL_DeleteContext(glContext);
    delete mesh;
    delete shader;
    delete object;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_Quit();
    return 0;
}