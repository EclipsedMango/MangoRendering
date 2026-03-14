
#include "PointLight.h"

PointLight::PointLight(const glm::vec3 position, const glm::vec3 color, const float intensity) {
    m_position = position;
    m_color = color;
    m_intensity = intensity;
}

PointLight::PointLight(const glm::vec3 position, const glm::vec3 color, const float intensity, const float radius) {
    m_position = position;
    m_color = color;
    m_intensity = intensity;
    m_radius = radius;
}

void PointLight::SetAttenuation(const float constant, const float linear, const float quadratic) {
    m_constant = constant;
    m_linear = linear;
    m_quadratic = quadratic;
}
