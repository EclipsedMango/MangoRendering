#ifndef MANGORENDERING_WINDOW_H
#define MANGORENDERING_WINDOW_H

#include <SDL3/SDL.h>
#include <string>

#include "glm/vec2.hpp"

class Window {
public:
    Window(const std::string& title, glm::ivec2 size, uint32_t flags);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    void MakeCurrent() const;
    void SwapBuffers() const;

    void SetTitle(const std::string& title);
    void SetSize(glm::ivec2 size);

    [[nodiscard]] glm::ivec2 GetSize() const { return m_Size; }
    [[nodiscard]] std::string GetTitle() const { return m_Title; }

    [[nodiscard]] SDL_Window* GetSDLWindow() const { return m_Window; }
    [[nodiscard]] SDL_GLContext GetContext() const { return m_Context; }

    [[nodiscard]] bool IsOpen() const { return m_IsOpen; }
    void Close() { m_IsOpen = false; }

private:
    SDL_Window* m_Window = nullptr;
    SDL_GLContext m_Context = nullptr;

    std::string m_Title;
    glm::ivec2 m_Size;
    bool m_IsOpen = false;
};


#endif //MANGORENDERING_WINDOW_H