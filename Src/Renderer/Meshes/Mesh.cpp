
#include "Mesh.h"

#include "glm/glm.hpp"

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) : m_vertices(vertices), m_indices(indices) {
    Upload();
    RegisterProperties();
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
}

void Mesh::Regenerate(const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices) {
    m_vertices = vertices;
    m_indices = indices;
    m_buffer.reset(); // destroy old GPU buffer
    m_buffer = std::make_unique<VertexArray>(m_vertices, m_indices);
    ComputeBounds();
}

void Mesh::RegisterProperties() {
    AddProperty("vertex_count",
        [this]() -> PropertyValue { return static_cast<int>(m_vertices.size()); },
        [](const PropertyValue&) {}
    );
    AddProperty("index_count",
        [this]() -> PropertyValue { return static_cast<int>(m_indices.size()); },
        [](const PropertyValue&) {}
    );
    AddProperty("bounds_center",
        [this]() -> PropertyValue { return m_boundsCenter; },
        [](const PropertyValue&) {}
    );
    AddProperty("bounds_radius",
        [this]() -> PropertyValue { return m_boundsRadius; },
        [](const PropertyValue&) {}
    );
}

void Mesh::ComputeBounds() {
    m_boundsCenter = glm::vec3(0.0f);
    for (const auto& v : m_vertices) {
        m_boundsCenter += v.position;
    }

    if (!m_vertices.empty()) {
        m_boundsCenter /= static_cast<float>(m_vertices.size());
    }

    m_boundsRadius = 0.0f;
    for (const auto& v : m_vertices) {
        const float dist = glm::distance(m_boundsCenter, v.position);
        if (dist > m_boundsRadius) {
            m_boundsRadius = dist;
        }
    }
}
