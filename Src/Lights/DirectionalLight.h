
#ifndef MANGORENDERING_DIRECTIONALLIGHT_H
#define MANGORENDERING_DIRECTIONALLIGHT_H

#include <glm/glm.hpp>

class DirectionalLight {
public:
    DirectionalLight(glm::vec3 direction, glm::vec3 color, float intensity);

    void SetDirection(const glm::vec3 direction) { m_direction = direction; }
    void SetColor(const glm::vec3 color) { m_color = color; }
    void SetIntensity(const float intensity) { m_intensity = intensity; }

    [[nodiscard]] glm::vec3 GetDirection() const { return m_direction; }
    [[nodiscard]] glm::vec3 GetColor() const { return m_color; }
    [[nodiscard]] float GetIntensity() const { return m_intensity; }

private:
    glm::vec3 m_direction{};
    glm::vec3 m_color{};
    float m_intensity;
};

#endif //MANGORENDERING_DIRECTIONALLIGHT_H