#include <iostream>

#include "RenderApi.h"
#include "glad/gl.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "SDL2/SDL.h"

int main() {
    RenderApi::Init();
    Window* window = RenderApi::CreateWindow("Mango", {SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED}, {500, 500}, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);

    SDL_GL_SetSwapInterval(1);

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

    mesh->Upload();

    shader->Bind();

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::rotate(model, glm::radians(45.0f), glm::vec3(1.0f, 1.0f, 0.0f));

    const glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 3.0f),  // camera position
        glm::vec3(0.0f, 0.0f, 0.0f),  // looking at origin
        glm::vec3(0.0f, 1.0f, 0.0f)   // up vector
    );

    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),  // fov
        500.0f / 500.0f,      // aspect ratio
        0.1f,                 // near plane
        100.0f                // far plane
    );

    SDL_Event event;
    while (window->IsOpen()) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                window->Close();
            }

            RenderApi::HandleResizeEvent(event);
        }

        window->MakeCurrent();
        RenderApi::ClearColour({0.12f, 0.12f, 0.12f, 1.0f});

        shader->Bind();
        shader->SetMatrix4("u_Model", model);
        shader->SetMatrix4("u_View", view);
        shader->SetMatrix4("u_Projection", projection);
        RenderApi::DrawMesh(*mesh, *shader);
        window->SwapBuffers();
    }

    // SDL_GL_DeleteContext(glContext);
    delete mesh;
    SDL_Quit();
    return 0;
}