
#include "SpotLight.h"

SpotLight::SpotLight(const glm::vec3 position, const glm::vec3 direction, const glm::vec3 color, const float intensity) {
    m_position = position;
    m_direction = direction;
    m_color = color;
    m_intensity = intensity;
}

void SpotLight::SetCutOffs(const float cutOffAngle, const float outerCutOffAngle) {
    // store cosines for shader comparison
    m_cutOff = glm::cos(glm::radians(cutOffAngle));
    m_outerCutOff = glm::cos(glm::radians(outerCutOffAngle));
}

void SpotLight::SetAttenuation(const float constant, const float linear, const float quadratic) {
    m_constant = constant;
    m_linear = linear;
    m_quadratic = quadratic;
}