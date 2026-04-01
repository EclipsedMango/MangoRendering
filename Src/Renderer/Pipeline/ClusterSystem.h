
#ifndef MANGORENDERING_CLUSTERSYSTEM_H
#define MANGORENDERING_CLUSTERSYSTEM_H

#include <memory>

#include "glm/glm.hpp"
#include "Nodes/CameraNode3d.h"
#include "Renderer/Shader.h"
#include "Renderer/Buffers/ShaderStorageBuffer.h"

class ClusterSystem {
public:
    static constexpr uint32_t CLUSTER_DIM_X = 16;
    static constexpr uint32_t CLUSTER_DIM_Y = 9;
    static constexpr uint32_t CLUSTER_DIM_Z = 24;
    static constexpr uint32_t NUM_CLUSTERS = CLUSTER_DIM_X * CLUSTER_DIM_Y * CLUSTER_DIM_Z;
    static constexpr uint32_t MAX_LIGHTS_PER_CLUSTER = 128;

    ClusterSystem();
    ~ClusterSystem();

    ClusterSystem(const ClusterSystem&) = delete;
    ClusterSystem& operator=(const ClusterSystem&) = delete;
    ClusterSystem(ClusterSystem&&) = delete;
    ClusterSystem& operator=(ClusterSystem&&) = delete;

    // called when the camera projection or viewport changes
    void Rebuild(const CameraNode3d& camera, const glm::vec2& viewportSize) const;

    // called once per frame after light SSBOs are uploaded
    void Cull(const ShaderStorageBuffer* pointLightSsbo, const ShaderStorageBuffer* spotLightSsbo);

    [[nodiscard]] ShaderStorageBuffer* GetClusterAabbSsbo() const { return m_clusterAabbSsbo.get(); }
    [[nodiscard]] ShaderStorageBuffer* GetLightIndexSsbo() const { return m_lightIndexSsbo.get(); }
    [[nodiscard]] ShaderStorageBuffer* GetLightGridSsbo() const;

    [[nodiscard]] ShaderStorageBuffer* GetGlobalCountSsbo() const { return m_globalCountSsbo.get(); }

private:
    std::shared_ptr<Shader> m_clusterShader;
    std::shared_ptr<Shader> m_cullShader;

    std::unique_ptr<ShaderStorageBuffer> m_clusterAabbSsbo;  // binding 4
    std::unique_ptr<ShaderStorageBuffer> m_lightIndexSsbo;   // binding 5
    std::unique_ptr<ShaderStorageBuffer> m_lightGridSsbo;    // binding 6
    std::unique_ptr<ShaderStorageBuffer> m_globalCountSsbo;  // binding 7
};


#endif //MANGORENDERING_CLUSTERSYSTEM_H