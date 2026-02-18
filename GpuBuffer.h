#ifndef MANGORENDERING_GPUBUFFER_H
#define MANGORENDERING_GPUBUFFER_H
#include <vector>

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
};

class GpuBuffer {
public:
    GpuBuffer(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
    ~GpuBuffer();

    GpuBuffer(const GpuBuffer&) = delete;
    GpuBuffer& operator=(const GpuBuffer&) = delete;

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


#endif //MANGORENDERING_GPUBUFFER_H