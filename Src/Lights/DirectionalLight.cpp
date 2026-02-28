
#include "DirectionalLight.h"

DirectionalLight::DirectionalLight(const glm::vec3 direction, const glm::vec3 color, const float intensity) {
    m_direction = direction;
    m_color = color;
    m_intensity = intensity;
}
