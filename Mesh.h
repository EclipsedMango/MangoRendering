#ifndef MANGORENDERING_MESH_H
#define MANGORENDERING_MESH_H

#include <memory>
#include <vector>
#include "VertexArray.h"

class Mesh {
public:
    Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    void Upload();
    [[nodiscard]] bool IsUploaded() const { return m_buffer != nullptr; }

    [[nodiscard]] const std::vector<Vertex>& GetVertices() const { return m_vertices; }
    [[nodiscard]] const std::vector<uint32_t>& GetIndices() const { return m_indices; }
    [[nodiscard]] VertexArray* GetBuffer() const { return m_buffer.get(); }

private:
    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;
    std::unique_ptr<VertexArray> m_buffer;
};


#endif //MANGORENDERING_MESH_H