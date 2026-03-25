
#ifndef MANGORENDERING_DIRECTIONALLIGHTNODE3D_H
#define MANGORENDERING_DIRECTIONALLIGHTNODE3D_H

#include "LightNode3d.h"
#include "Renderer/Lights/DirectionalLight.h"

class DirectionalLightNode3d : public LightNode3d {
public:
    DirectionalLightNode3d();
    DirectionalLightNode3d(glm::vec3 direction, glm::vec3 color, float intensity);
    ~DirectionalLightNode3d() override = default;

    void SetLightDirection(const glm::vec3 direction) { m_light.SetDirection(direction); }

    [[nodiscard]] std::string GetNodeType() const override { return "DirectionalLightNode3d"; }
    [[nodiscard]] DirectionalLight* GetLight() { return &m_light; }

    void Process(float deltaTime) override;
    void SyncLight() override;

private:
    DirectionalLight m_light;
};


#endif //MANGORENDERING_DIRECTIONALLIGHTNODE3D_H