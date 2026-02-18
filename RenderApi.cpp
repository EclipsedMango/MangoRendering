#include "RenderApi.h"

#include <SDL_video.h>
#include <stdexcept>
#include "glad/gl.h"

bool RenderApi::m_gladInitialized = false;
std::vector<Window*> RenderApi::m_windows {};

void RenderApi::Init() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
}

Window* RenderApi::CreateWindow(const char* title, const glm::vec2 pos, const glm::vec2 size, const Uint32 flags) {
    Window* window = new Window(title, pos, size, flags);
    window->MakeCurrent();

    if (!m_gladInitialized) {
        if (!gladLoadGL(reinterpret_cast<GLADloadfunc>(SDL_GL_GetProcAddress))) {
            throw std::runtime_error("Failed to initialize Glad");
        }

        m_gladInitialized = true;
    }

    glEnable(GL_DEPTH_TEST);
    m_windows.push_back(window);
    return window;
}
