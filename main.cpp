#include <iostream>

#include "RenderApi.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Camera.h"
#include "SDL2/SDL.h"

int main() {
    RenderApi::Init();
    Window* window = RenderApi::CreateWindow("Mango", {SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED}, {500, 500}, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);

    SDL_GL_SetSwapInterval(1);

    bool mouseCaptured = true;
    SDL_SetRelativeMouseMode(static_cast<SDL_bool>(mouseCaptured));

    Mesh* mesh = new Mesh(
        {
            // Back face
            {{-0.5f, -0.5f, -0.5f}, {}, {0.0f, 0.0f}},
            {{ 0.5f, -0.5f, -0.5f}, {}, {1.0f, 0.0f}},
            {{ 0.5f,  0.5f, -0.5f}, {}, {1.0f, 1.0f}},
            {{-0.5f,  0.5f, -0.5f}, {}, {0.0f, 1.0f}},

            // Front face
            {{-0.5f, -0.5f,  0.5f}, {}, {0.0f, 0.0f}},
            {{ 0.5f, -0.5f,  0.5f}, {}, {1.0f, 0.0f}},
            {{ 0.5f,  0.5f,  0.5f}, {}, {1.0f, 1.0f}},
            {{-0.5f,  0.5f,  0.5f}, {}, {0.0f, 1.0f}},

            // Left face
            {{-0.5f, -0.5f, -0.5f}, {}, {1.0f, 0.0f}},
            {{-0.5f, -0.5f,  0.5f}, {}, {0.0f, 0.0f}},
            {{-0.5f,  0.5f,  0.5f}, {}, {0.0f, 1.0f}},
            {{-0.5f,  0.5f, -0.5f}, {}, {1.0f, 1.0f}},

            // Right face
            {{ 0.5f, -0.5f, -0.5f}, {}, {0.0f, 0.0f}},
            {{ 0.5f, -0.5f,  0.5f}, {}, {1.0f, 0.0f}},
            {{ 0.5f,  0.5f,  0.5f}, {}, {1.0f, 1.0f}},
            {{ 0.5f,  0.5f, -0.5f}, {}, {0.0f, 1.0f}},

            // Bottom face
            {{-0.5f, -0.5f, -0.5f}, {}, {0.0f, 1.0f}},
            {{ 0.5f, -0.5f, -0.5f}, {}, {1.0f, 1.0f}},
            {{ 0.5f, -0.5f,  0.5f}, {}, {1.0f, 0.0f}},
            {{-0.5f, -0.5f,  0.5f}, {}, {0.0f, 0.0f}},

            // Top face
            {{-0.5f,  0.5f, -0.5f}, {}, {0.0f, 1.0f}},
            {{ 0.5f,  0.5f, -0.5f}, {}, {1.0f, 1.0f}},
            {{ 0.5f,  0.5f,  0.5f}, {}, {1.0f, 0.0f}},
            {{-0.5f,  0.5f,  0.5f}, {}, {0.0f, 0.0f}},
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
    const Shader* shader = new Shader("Shaders/test.vert", "Shaders/test.frag");
    Camera* camera = new Camera({0, 0, 3}, 45.0f, 500.0f / 500.0f, 0.1f, 100.0f);

    mesh->Upload();
    shader->Bind();

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::rotate(model, glm::radians(45.0f), glm::vec3(1.0f, 1.0f, 0.0f));

    uint32_t lastTime = SDL_GetTicks();
    SDL_Event event;
    while (window->IsOpen()) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                window->Close();
            }

            if (event.type == SDL_MOUSEMOTION) {
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

        shader->Bind();
        shader->SetMatrix4("u_Model", model);
        shader->SetMatrix4("u_View", camera->GetViewMatrix());
        shader->SetMatrix4("u_Projection", camera->GetProjectionMatrix());
        RenderApi::DrawMesh(*mesh, *shader);
        window->SwapBuffers();
    }

    // SDL_GL_DeleteContext(glContext);
    delete mesh;
    SDL_Quit();
    return 0;
}