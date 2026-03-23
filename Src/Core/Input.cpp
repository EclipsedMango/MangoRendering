
#include "Input.h"

std::array<bool, SDL_SCANCODE_COUNT> Input::m_current = {};
std::array<bool, SDL_SCANCODE_COUNT> Input::m_previous = {};

glm::vec2 Input::m_mouseDelta = {};
glm::vec2 Input::m_mousePosition = {};

Uint32 Input::m_mouseButtonsCurrent = 0;
Uint32 Input::m_mouseButtonsPrevious = 0;

float Input::m_mouseWheelX = 0.0f;
float Input::m_mouseWheelY = 0.0f;

bool Input::m_mouseDeltaEnabled = true;

void Input::BeginFrame() {
    m_previous = m_current;
    m_mouseButtonsPrevious = m_mouseButtonsCurrent;
    m_mouseDelta = {};

    m_mouseWheelX = 0.0f;
    m_mouseWheelY = 0.0f;
}

void Input::ProcessEvent(const SDL_Event& event) {
    switch (event.type) {
        case SDL_EVENT_KEY_DOWN: {
            m_current[event.key.scancode] = true;
            break;
        }

        case SDL_EVENT_KEY_UP: {
            m_current[event.key.scancode] = false;
            break;
        }

        case SDL_EVENT_MOUSE_MOTION: {
            m_mousePosition = {
                static_cast<float>(event.motion.x),
                static_cast<float>(event.motion.y)
            };

            if (m_mouseDeltaEnabled) {
                m_mouseDelta.x += static_cast<float>(event.motion.xrel);
                m_mouseDelta.y += static_cast<float>(event.motion.yrel);
            }

            break;
        }

        case SDL_EVENT_MOUSE_WHEEL: {
            float x = event.wheel.x;
            float y = event.wheel.y;

            if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
                x = -x;
                y = -y;
            }

            m_mouseWheelX += x;
            m_mouseWheelY += y;
            break;
        }

        default: {
            break;
        }
    }
}

void Input::EndFrame() {
    int numKeys = 0;
    const bool* sdlKeys = SDL_GetKeyboardState(&numKeys);

    for (int i = 0; i < numKeys && i < SDL_SCANCODE_COUNT; ++i) {
        m_current[i] = sdlKeys[i];
    }

    float mx = 0.0f;
    float my = 0.0f;
    SDL_GetMouseState(&mx, &my);
    m_mouseButtonsCurrent = SDL_GetMouseState(&mx, &my);
    m_mousePosition = { mx, my };
}

void Input::SetMouseDeltaEnabled(const bool enabled) {
    m_mouseDeltaEnabled = enabled;
    if (!enabled) {
        m_mouseDelta = {};
    }
}

bool Input::IsKeyHeld(const SDL_Scancode key) {
    return m_current[key];
}

bool Input::IsKeyJustPressed(const SDL_Scancode key) {
    return m_current[key] && !m_previous[key];
}

bool Input::IsKeyJustPressedWithMod(const SDL_Scancode key, const SDL_Keymod mod) {
    return IsKeyJustPressed(key) && (SDL_GetModState() & mod);
}

bool Input::IsKeyJustReleased(const SDL_Scancode key) {
    return !m_current[key] && m_previous[key];
}

bool Input::IsMouseButtonHeld(const int button) {
    return (m_mouseButtonsCurrent & SDL_BUTTON_MASK(button)) != 0;
}

bool Input::IsMouseButtonJustPressed(const int button) {
    const Uint32 mask = SDL_BUTTON_MASK(button);
    return m_mouseButtonsCurrent & mask && !(m_mouseButtonsPrevious & mask);
}

bool Input::IsMouseButtonJustReleased(const int button) {
    const Uint32 mask = SDL_BUTTON_MASK(button);
    return !(m_mouseButtonsCurrent & mask) && m_mouseButtonsPrevious & mask;
}
