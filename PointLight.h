
#ifndef MANGORENDERING_POINTLIGHT_H
#define MANGORENDERING_POINTLIGHT_H

#include <glm/glm.hpp>

class PointLight {
public:
    PointLight(glm::vec3 position, glm::vec3 color, float intensity);

    void SetPosition(const glm::vec3 position) { m_position = position; }
    void SetColor(const glm::vec3 color) { m_color = color; }
    void SetIntensity(const float intensity) { m_intensity = intensity; }
    void SetAttenuation(float constant, float linear, float quadratic);

    [[nodiscard]] glm::vec3 GetPosition() const { return m_position; }
    [[nodiscard]] glm::vec3 GetColor() const { return m_color; }
    [[nodiscard]] float GetIntensity() const { return m_intensity; }

    [[nodiscard]] float GetConstant() const { return m_constant; }
    [[nodiscard]] float GetLinear() const { return m_linear; }
    [[nodiscard]] float GetQuadratic() const { return m_quadratic; }

private:
    glm::vec3 m_position{};
    glm::vec3 m_color{};
    float m_intensity;

    float m_constant = 1.0f;
    float m_linear = 0.09f;
    float m_quadratic = 0.032f;
};

#endif //MANGORENDERING_POINTLIGHT_H