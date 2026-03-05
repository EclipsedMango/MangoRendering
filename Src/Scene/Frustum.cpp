
#include "Frustum.h"

void Frustum::ExtractFromMatrix(const glm::mat4 &viewProj) {
    const glm::mat4 m = glm::transpose(viewProj);

    m_planes[0] = NormalizePlane(m[3] + m[0]); // left
    m_planes[1] = NormalizePlane(m[3] - m[0]); // right
    m_planes[2] = NormalizePlane(m[3] + m[1]); // bottom
    m_planes[3] = NormalizePlane(m[3] - m[1]); // top
    m_planes[4] = NormalizePlane(m[3] + m[2]); // near
    m_planes[5] = NormalizePlane(m[3] - m[2]); // far
}

bool Frustum::IntersectsSphere(const glm::vec3 &center, const float radius) const {
    for (const auto& plane: m_planes) {
        if (glm::dot(glm::vec3(plane.x, plane.y, plane.z), center) + plane.w < -radius) {
            return false;
        }
    }

    return true;
}

glm::vec4 Frustum::NormalizePlane(const glm::vec4 plane) {
    const float length = glm::length(glm::vec3(plane));
    return plane / length;
}

// debug tool to figure out which plane is culling
int Frustum::FailingPlane(const glm::vec3& center, const float radius) const {
    for (int i = 0; i < 6; i++) {
        if (glm::dot(glm::vec3(m_planes[i]), center) + m_planes[i].w < -radius) {
            return i;
        }
    }
    return -1;
}
