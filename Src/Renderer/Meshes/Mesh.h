#ifndef MANGORENDERING_MESH_H
#define MANGORENDERING_MESH_H

#include <memory>
#include <vector>

#include "Renderer/VertexArray.h"
#include "Core/PropertyHolder.h"

class Mesh : public PropertyHolder {
public:
    Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
    virtual ~Mesh() = default;

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    void Upload();
    void Regenerate(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

    [[nodiscard]] bool IsUploaded() const { return m_buffer != nullptr; }

    [[nodiscard]] const std::vector<Vertex>& GetVertices() const { return m_vertices; }
    [[nodiscard]] const std::vector<uint32_t>& GetIndices() const { return m_indices; }
    [[nodiscard]] VertexArray* GetBuffer() const { return m_buffer.get(); }

    [[nodiscard]] glm::vec3 GetBoundsCenter() const { return m_boundsCenter; }
    [[nodiscard]] float GetBoundsRadius() const { return m_boundsRadius; }

protected:
    Mesh() { RegisterProperties(); }

private:
    void RegisterProperties();
    void ComputeBounds();

    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;
    std::unique_ptr<VertexArray> m_buffer;

    glm::vec3 m_boundsCenter = glm::vec3(0.0f);
    float m_boundsRadius = 0.0f;
};


#endif //MANGORENDERING_MESH_H