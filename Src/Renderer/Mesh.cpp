
#include "Mesh.h"

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) : m_vertices(vertices), m_indices(indices) {
    Upload();
}

void Mesh::Upload() {
    if (m_buffer) {
        return;
    }

    m_buffer = std::make_unique<VertexArray>(m_vertices, m_indices);
}
