#include "Window.h"
#include <stdexcept>

Window::Window(const std::string& title, const glm::ivec2 position, const glm::ivec2 size, const uint32_t flags) : m_Title(title), m_Size(size), m_Position(position) {
    m_Window = SDL_CreateWindow(title.c_str(), position.x, position.y, size.x, size.y, flags | SDL_WINDOW_OPENGL);

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
        SDL_GL_DeleteContext(m_Context);
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

void Window::SetPosition(const glm::ivec2 position) {
    m_Position = position;
    SDL_SetWindowPosition(m_Window, position.x, position.y);
}

glm::ivec2 Window::GetSize() const { return m_Size; }
glm::ivec2 Window::GetPosition() const { return m_Position; }
std::string Window::GetTitle() const { return m_Title; }
