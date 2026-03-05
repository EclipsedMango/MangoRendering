
#ifndef MANGORENDERING_FRUSTUM_H
#define MANGORENDERING_FRUSTUM_H

#include "glm/glm.hpp"

class Frustum {
public:
    void ExtractFromMatrix(const glm::mat4& viewProj);
    [[nodiscard]] bool IntersectsSphere(const glm::vec3& center, float radius) const;
    [[nodiscard]] int FailingPlane(const glm::vec3& center, float radius) const;
    static glm::vec4 NormalizePlane(glm::vec4 plane);

private:
    glm::vec4 m_planes[6];
};

#endif //MANGORENDERING_FRUSTUM_H