#ifndef MANGORENDERING_VERTEXARRAY_H
#define MANGORENDERING_VERTEXARRAY_H

#include <vector>

#include "glm/glm.hpp"

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
    glm::vec4 tangent;
    glm::uvec4 joints = glm::uvec4(0);
    glm::vec4 weights = glm::vec4(0.0f);
};

class VertexArray {
public:
    VertexArray(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
    ~VertexArray();

    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;

    void Bind() const;
    static void Unbind() ;

    [[nodiscard]] int GetIndexCount() const { return m_IndexCount; }

private:
    void Setup(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

    unsigned int m_VAO = 0;
    unsigned int m_VBO = 0;
    unsigned int m_EBO = 0;
    int m_IndexCount = 0;
};


#endif //MANGORENDERING_VERTEXARRAY_H
