
#ifndef MANGORENDERING_SHADOWRENDERER_H
#define MANGORENDERING_SHADOWRENDERER_H

#include <array>
#include <unordered_map>
#include <vector>

#include "Glad/glad.h"
#include "glm/glm.hpp"
#include "Nodes/CameraNode3d.h"
#include "Nodes/MeshNode3d.h"
#include "Renderer/Shader.h"
#include "Renderer/Shadows/CascadedShadowMap.h"
#include "Renderer/Shadows/PointLightShadowMap.h"
#include "Renderer/Lights/DirectionalLight.h"
#include "Renderer/Lights/GpuLights.h"
#include "Renderer/Lights/PointLight.h"
#include "Renderer/Buffers/ShaderStorageBuffer.h"

struct ShadowedPointLightDebug {
    uint32_t lightIndex = 0;
    uint32_t slot = 0;
    float score = 0.0f;
    float radius = 0.0f;
    float farPlane = 0.0f;
    glm::vec3 position { 0.0f };
    float distanceToCamera = 0.0f;
};

class ShadowRenderer {
public:
    static constexpr uint32_t MAX_SHADOWED_POINT_LIGHTS = 8;
    static constexpr uint32_t POINT_SHADOW_RES = 512;
    static constexpr uint32_t MAX_DIR_LIGHTS = 1;

    // texture slots 7, 8, 9, 10 are reserved for CSM, point light cube maps start at 15.
    static constexpr int CSM_TEXTURE_SLOT_BASE = 7;
    static constexpr int POINT_SHADOW_SLOT = 15;

    static constexpr int MAX_POINT_LIGHT_SHADOW_DISTANCE = 50;

    ShadowRenderer();
    ~ShadowRenderer();

    ShadowRenderer(const ShadowRenderer&) = delete;
    ShadowRenderer& operator=(const ShadowRenderer&) = delete;
    ShadowRenderer(ShadowRenderer&&) = delete;
    ShadowRenderer& operator=(ShadowRenderer&&) = delete;

    // light registration
    void AddDirectionalLight(DirectionalLight* light);
    void RemoveDirectionalLight(DirectionalLight* light);

    // called once per frame from RenderApi::Flush()
    void BuildShadowDrawItems(const std::vector<MeshNode3d*>& renderQueue);
    void RenderDirectionalShadows(const CameraNode3d& camera, const glm::vec2& viewportSize);
    void RenderPointLightShadows(const CameraNode3d& camera, const std::vector<PointLight*>& pointLights, const glm::vec2& viewportSize);

    // binds shadow uniforms onto whatever shader is currently in use for the main pass
    void BindShadowUniforms(const Shader& shader) const;

    // binds the point shadow cube array at GL_TEXTURE15
    void BindPointShadowTexture() const;

    [[nodiscard]] const std::vector<CascadedShadowMap*>& GetCascadedShadowMaps() const { return m_cascadedShadowMaps; }
    [[nodiscard]] const std::vector<ShadowedPointLightDebug>&  GetShadowedPointLightsDebug() const { return m_shadowedPointLightsDebug; }
    [[nodiscard]] uint32_t GetShadowDrawCallCount() const { return m_shadowDrawCallCount; }
    [[nodiscard]] uint32_t GetShadowedPointLightCount() const { return static_cast<uint32_t>(m_shadowedPointLightsDebug.size()); }

    // reset per-frame stats (called by RenderApi at the start of Flush())
    void ResetStats() { m_shadowDrawCallCount = 0; }

private:
    struct BatchKey {
        uint64_t meshId = 0;
        uint64_t materialId = 0;

        bool operator==(const BatchKey& other) const {
            return meshId == other.meshId && materialId == other.materialId;
        }
    };

    struct BatchKeyHash {
        size_t operator()(const BatchKey& key) const {
            const size_t h1 = std::hash<uint64_t>{}(key.meshId);
            const size_t h2 = std::hash<uint64_t>{}(key.materialId);
            return h1 ^ (h2 << 1);
        }
    };

    struct ShadowDrawItem {
        const MeshNode3d* node = nullptr;
        const Mesh* mesh = nullptr;
        const Material* material = nullptr;
        BatchKey batchKey{};
        glm::mat4 model {1.0f};
        glm::mat4 normalMatrix {1.0f};
        glm::vec3 worldCenter {0.0f};
        float worldRadius = 0.0f;
        bool singleDraw = false;
    };

    struct InstanceDrawData {
        glm::mat4 model;
        glm::mat4 normalMatrix;
    };

    struct ShadowCandidate {
        uint32_t index;
        float score;
    };

    void BuildPointLightBatches(const glm::vec3& lightPos, float lightRadius);
    void EnsurePointShadowVpUniformLocation();
    void EnsurePointShadowMetaBuffer(size_t pointLightCount);
    void EnsureInstanceBuffer(size_t instanceCount);
    static float ScorePointLight(const PointLight* light, const CameraNode3d& camera);
    static void BuildPointShadowFaceMatrices(const glm::vec3& lightPos, float nearPlane, float farPlane, glm::mat4 outVP[6]);

    // directional shadow resources
    std::shared_ptr<Shader> m_shadowDepthShader;
    std::vector<DirectionalLight*> m_directionalLights;
    std::vector<CascadedShadowMap*> m_cascadedShadowMaps;   // parallel to m_directionalLights

    // point shadow resources
    std::shared_ptr<Shader> m_pointShadowDepthShader;
    std::unique_ptr<PointLightShadowMap> m_pointShadowMap;
    std::unique_ptr<ShaderStorageBuffer> m_pointShadowMetaSsbo;  // binding 8
    std::unique_ptr<ShaderStorageBuffer> m_instanceSsbo;  // binding 12
    GLint m_pointVpArrayUniformLocation = -2;

    uint32_t m_shadowDrawCallCount = 0;
    std::vector<ShadowedPointLightDebug> m_shadowedPointLightsDebug;

    std::vector<ShadowDrawItem> m_shadowDrawItems;
    std::vector<const ShadowDrawItem*> m_singleDrawItems;
    std::unordered_map<BatchKey, std::vector<const ShadowDrawItem*>, BatchKeyHash> m_instancedBatches;
    std::vector<const ShadowDrawItem*> m_filteredSingleDrawItems;
    std::unordered_map<BatchKey, std::vector<const ShadowDrawItem*>, BatchKeyHash> m_filteredInstancedBatches;
    std::vector<InstanceDrawData> m_instanceDataScratch;
    std::array<glm::mat4, 6> m_pointLightVpScratch{};
    std::vector<ShadowCandidate> m_shadowCandidatesScratch;
    std::vector<GPUPointShadowMeta> m_pointShadowMetaScratch;
};


#endif //MANGORENDERING_SHADOWRENDERER_H
