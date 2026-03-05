
#include "Mesh.h"

#include "glm/glm.hpp"

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) : m_vertices(vertices), m_indices(indices) {
    Upload();
}

void Mesh::Upload() {
    if (m_buffer) {
        return;
    }

    m_buffer = std::make_unique<VertexArray>(m_vertices, m_indices);

    for (const auto & vert : m_vertices) {
        m_boundsCenter += vert.position;
    }

    m_boundsCenter /= m_vertices.size();

    float max_dist = 0.0f;
    for (auto & vert : m_vertices) {
        const float dist = glm::distance(m_boundsCenter, vert.position);
        if (dist > max_dist) {
            max_dist = dist;
        }
    }

    m_boundsRadius = max_dist;

    printf("Bounds Rad: %f\n", m_boundsRadius);
    printf("Bounds: %f, %f, %f\n", m_boundsCenter.x, m_boundsCenter.y, m_boundsCenter.z);
}
