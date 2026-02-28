#include "Window.h"
#include <stdexcept>

Window::Window(const std::string& title, const glm::ivec2 size, const uint32_t flags) : m_Title(title), m_Size(size) {
    m_Window = SDL_CreateWindow(title.c_str(), size.x, size.y, flags | SDL_WINDOW_OPENGL);

    if (!m_Window) {
        throw std::runtime_error(std::string("Failed to create window: ") + SDL_GetError());
    }

    m_Context = SDL_GL_CreateContext(m_Window);

    if (!m_Context) {
        throw std::runtime_error(std::string("Failed to create GL context: ") + SDL_GetError());
    }

    m_IsOpen = true;
}

Window::~Window() {
    if (m_Context) {
        SDL_GL_DestroyContext(m_Context);
    }

    if (m_Window) {
        SDL_DestroyWindow(m_Window);
    }
}

// set up an OpenGL context for rendering into an OpenGL window
void Window::MakeCurrent() const {
    SDL_GL_MakeCurrent(m_Window, m_Context);
}

void Window::SwapBuffers() const {
    SDL_GL_SwapWindow(m_Window);
}

void Window::SetTitle(const std::string& title) {
    m_Title = title;
    SDL_SetWindowTitle(m_Window, title.c_str());
}

void Window::SetSize(const glm::ivec2 size) {
    m_Size = size;
    SDL_SetWindowSize(m_Window, size.x, size.y);
}
