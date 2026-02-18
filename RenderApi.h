#ifndef MANGORENDERING_RENDERAPI_H
#define MANGORENDERING_RENDERAPI_H

#include <SDL.h>
#include <vector>

#include "Window.h"
#include "glm/vec2.hpp"

class RenderApi {
public:
    static void Init();
    static Window* CreateWindow(const char* title, glm::vec2 pos, glm::vec2 size, Uint32 flags);

protected:
    RenderApi() = default;

private:
    static bool m_gladInitialized;
    static std::vector<Window*> m_windows;
};


#endif //MANGORENDERING_RENDERAPI_H