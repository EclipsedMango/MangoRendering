
#include "Mesh.h"

#include <cstdint>

#include "glm/glm.hpp"
#include "Renderer/Buffers/ShaderStorageBuffer.h"

namespace {
    struct GpuSkinningInputVertex {
        glm::vec4 position;
        glm::vec4 normal;
        glm::vec4 tangent;
        glm::uvec4 joints;
        glm::vec4 weights;
    };

    constexpr uint32_t kSkinningInputBinding = 10;
}

REGISTER_PROPERTY_TYPE(Mesh)

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) : m_vertices(vertices), m_indices(indices) {
    ComputeSkinWeightsUsage();
    m_skinningSourceUploaded = false;
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
    ComputeSkinWeightsUsage();
    m_skinningSourceUploaded = false;
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

void Mesh::ComputeSkinWeightsUsage() {
    m_hasSkinWeights = false;
    for (const auto& v : m_vertices) {
        if (v.weights.x > 0.0f || v.weights.y > 0.0f || v.weights.z > 0.0f || v.weights.w > 0.0f) {
            m_hasSkinWeights = true;
            break;
        }
    }
}

ShaderStorageBuffer* Mesh::GetSkinningSourceBuffer() const {
    if (!m_hasSkinWeights || m_vertices.empty()) {
        return nullptr;
    }

    EnsureSkinningSourceBuffer();
    return m_skinningSourceSsbo.get();
}

void Mesh::EnsureSkinningSourceBuffer() const {
    if (m_vertices.empty()) {
        return;
    }

    const size_t requiredBytes = m_vertices.size() * sizeof(GpuSkinningInputVertex);
    if (!m_skinningSourceSsbo || m_skinningSourceSsbo->GetSize() < requiredBytes) {
        m_skinningSourceSsbo = std::make_unique<ShaderStorageBuffer>(requiredBytes, kSkinningInputBinding);
        m_skinningSourceUploaded = false;
    }

    if (m_skinningSourceUploaded) {
        return;
    }

    std::vector<GpuSkinningInputVertex> gpuVertices;
    gpuVertices.reserve(m_vertices.size());
    for (const Vertex& vertex : m_vertices) {
        gpuVertices.push_back({
            glm::vec4(vertex.position, 1.0f),
            glm::vec4(vertex.normal, 0.0f),
            vertex.tangent,
            vertex.joints,
            vertex.weights
        });
    }

    m_skinningSourceSsbo->SetData(gpuVertices.data(), requiredBytes, 0);
    m_skinningSourceUploaded = true;
}
