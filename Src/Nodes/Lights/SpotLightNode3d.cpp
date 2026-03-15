
#include "SpotLightNode3d.h"

SpotLightNode3d::SpotLightNode3d(const glm::vec3 position, const glm::vec3 direction, const glm::vec3 color, const float intensity) : LightNode3d(color, intensity), m_light(position, direction, color, intensity) {
    SetPosition(position);
    SetRotation(direction);
}

void SpotLightNode3d::Process() {
    m_light.SetPosition(GetPosition());
    m_light.SetDirection(GetRotation());
    m_light.SetColor(GetColor());
    m_light.SetIntensity(GetIntensity());
}