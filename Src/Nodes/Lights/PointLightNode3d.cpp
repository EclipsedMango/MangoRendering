
#include "PointLightNode3d.h"

PointLightNode3d::PointLightNode3d(const glm::vec3 position, const glm::vec3 color, const float intensity, const float radius) : LightNode3d(color, intensity), m_light(position, color, intensity, radius) {
    SetPosition(position);
    SetName("PointLightNode3d");
    AddProperty("color",
        [this]() -> PropertyValue { return GetColor(); },
        [this](const PropertyValue& v) { SetColor(std::get<glm::vec3>(v)); }
    );
    AddProperty("intensity",
        [this]() -> PropertyValue { return GetIntensity(); },
        [this](const PropertyValue& v) { SetIntensity(std::get<float>(v)); }
    );
    AddProperty("radius",
        [this]() -> PropertyValue { return GetLight()->GetRadius(); },
        [this](const PropertyValue& v) { SetRadius(std::get<float>(v)); }
    );
}

void PointLightNode3d::Process(float deltaTime) {
    m_light.SetPosition(GetPosition());
    m_light.SetColor(GetColor());
    m_light.SetIntensity(GetIntensity());
}

void PointLightNode3d::SyncLight() {
    const glm::vec3 worldPos = glm::vec3(GetWorldMatrix()[3]);
    m_light.SetPosition(worldPos);
    m_light.SetColor(GetColor());
    m_light.SetIntensity(GetIntensity());
}
