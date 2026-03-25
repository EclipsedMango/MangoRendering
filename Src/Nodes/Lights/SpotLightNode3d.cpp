
#include "SpotLightNode3d.h"

REGISTER_NODE_TYPE(SpotLightNode3d)

SpotLightNode3d::SpotLightNode3d() : SpotLightNode3d(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(1.0f), 1.0f) {}

SpotLightNode3d::SpotLightNode3d(const glm::vec3 position, const glm::vec3 direction, const glm::vec3 color, const float intensity) : LightNode3d(color, intensity), m_light(position, direction, color, intensity) {
    SetPosition(position);
    glm::vec3 up = fabsf(direction.y) > 0.999f ? glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);
    SetRotation(glm::quatLookAt(glm::normalize(direction), up));
    SetName("SpotLightNode3d");
}

void SpotLightNode3d::Process(float deltaTime) {
    m_light.SetColor(GetColor());
    m_light.SetIntensity(GetIntensity());
}

void SpotLightNode3d::SyncLight() {
    const glm::vec3 worldPos = glm::vec3(GetWorldMatrix()[3]);
    const glm::vec3 dir = glm::normalize(glm::vec3(GetWorldMatrix() * glm::vec4(0, 0, -1, 0)));
    m_light.SetPosition(worldPos);
    m_light.SetDirection(dir);
    m_light.SetColor(GetColor());
    m_light.SetIntensity(GetIntensity());
}
