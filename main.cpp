#include <iostream>

#include "RenderApi.h"
#include "glad/gl.h"
#include "SDL2/SDL.h"

int main() {
    RenderApi::Init();
    Window* window = RenderApi::CreateWindow("Mango", {SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED}, {500, 500}, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);

    SDL_GL_SetSwapInterval(1);

    SDL_Event event;

    while (window->IsOpen()) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                window->Close();
            }

            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                window->Close();
            }
        }

        window->MakeCurrent();
        glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        window->SwapBuffers();
    }

    // SDL_GL_DeleteContext(glContext);
    SDL_Quit();
    return 0;
}