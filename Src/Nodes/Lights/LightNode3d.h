
#ifndef MANGORENDERING_LIGHTNODE3D_H
#define MANGORENDERING_LIGHTNODE3D_H

#include "Nodes/Node3d.h"

class LightNode3d : public Node3d {
public:
    LightNode3d(glm::vec3 color, float intensity);

    void SetColor(const glm::vec3 color) { m_color = color; }
    void SetIntensity(const float intensity) { m_intensity = intensity; }

    [[nodiscard]] glm::vec3 GetColor() const { return m_color; }
    [[nodiscard]] float GetIntensity() const { return m_intensity; }

private:
    glm::vec3 m_color{};
    float m_intensity = 1.0f;
};


#endif //MANGORENDERING_LIGHTNODE3D_H