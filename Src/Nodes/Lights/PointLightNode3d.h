
#ifndef MANGORENDERING_POINTLIGHTNODE3D_H
#define MANGORENDERING_POINTLIGHTNODE3D_H
#include "LightNode3d.h"
#include "PointLight.h"


class PointLightNode3d : public LightNode3d {
public:
    PointLightNode3d(glm::vec3 position, glm::vec3 color, float intensity, float radius = 8.0f);
    ~PointLightNode3d() override = default;

    void SetRadius(const float radius) { m_light.SetRadius(radius); }
    void SetAttenuation(const float c, const float l, const float q) { m_light.SetAttenuation(c, l, q); }

    [[nodiscard]] PointLight* GetLight() { return &m_light; }

    void Process() override;

private:
    PointLight m_light;
};


#endif //MANGORENDERING_POINTLIGHTNODE3D_H