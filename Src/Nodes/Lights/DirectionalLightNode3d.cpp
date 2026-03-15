
#include "DirectionalLightNode3d.h"

DirectionalLightNode3d::DirectionalLightNode3d(const glm::vec3 direction, const glm::vec3 color, const float intensity) : LightNode3d(color, intensity), m_light(direction, color, intensity) {
    SetRotation(direction);
}

void DirectionalLightNode3d::Process() {
    m_light.SetDirection(GetRotation());
    m_light.SetColor(GetColor());
    m_light.SetIntensity(GetIntensity());
}