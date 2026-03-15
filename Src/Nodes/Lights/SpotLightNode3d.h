
#ifndef MANGORENDERING_SPOTLIGHTNODE3D_H
#define MANGORENDERING_SPOTLIGHTNODE3D_H

#include "LightNode3d.h"
#include "SpotLight.h"

class SpotLightNode3d : public LightNode3d {
public:
    SpotLightNode3d(glm::vec3 position, glm::vec3 direction, glm::vec3 color, float intensity);
    ~SpotLightNode3d() override = default;

    void SetCutOffs(const float inner, const float outer) { m_light.SetCutOffs(inner, outer); }
    void SetAttenuation(const float c, const float l, const float q) { m_light.SetAttenuation(c, l, q); }

    [[nodiscard]] SpotLight* GetLight() { return &m_light; }

    void Process() override;

private:
    SpotLight m_light;
};


#endif //MANGORENDERING_SPOTLIGHTNODE3D_H