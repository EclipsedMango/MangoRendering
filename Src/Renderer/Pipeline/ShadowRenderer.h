
#ifndef MANGORENDERING_SHADOWRENDERER_H
#define MANGORENDERING_SHADOWRENDERER_H

#include "glm/glm.hpp"
#include "Nodes/MeshNode3d.h"
#include "Renderer/Shader.h"
#include "Renderer/CascadedShadowMap.h"
#include "Renderer/PointLightShadowMap.h"
#include "Scene/Camera.h"
#include "Scene/Object.h"
#include "Nodes/Lights/DirectionalLight.h"
#include "Nodes/Lights/PointLight.h"
#include "Renderer/Buffers/ShaderStorageBuffer.h"

struct ShadowedPointLightDebug {
    uint32_t  lightIndex      = 0;
    uint32_t  slot            = 0;
    float     score           = 0.0f;
    float     radius          = 0.0f;
    float     farPlane        = 0.0f;
    glm::vec3 position        { 0.0f };
    float     distanceToCamera = 0.0f;
};

class ShadowRenderer {
public:
    static constexpr uint32_t CSM_RESOLUTION             = 2048;
    static constexpr uint32_t MAX_SHADOWED_POINT_LIGHTS  = 8;
    static constexpr uint32_t POINT_SHADOW_RES           = 1024;
    static constexpr uint32_t MAX_DIR_LIGHTS             = 1;

    ShadowRenderer();
    ~ShadowRenderer();

    ShadowRenderer(const ShadowRenderer&)            = delete;
    ShadowRenderer& operator=(const ShadowRenderer&) = delete;
    ShadowRenderer(ShadowRenderer&&)                 = delete;
    ShadowRenderer& operator=(ShadowRenderer&&)      = delete;

    // light registration
    void AddDirectionalLight(DirectionalLight* light);
    void RemoveDirectionalLight(DirectionalLight* light);

    // called once per frame from RenderApi::Flush()
    void RenderDirectionalShadows(const Camera& camera, const std::vector<MeshNode3d*>& renderQueue, const glm::vec2& viewportSize);
    void RenderPointLightShadows(const Camera& camera, const std::vector<PointLight*>& pointLights, const std::vector<MeshNode3d*>& renderQueue, const glm::vec2& viewportSize);

    // binds shadow uniforms onto whatever shader is currently in use for the main pass
    void BindShadowUniforms(const Shader& shader) const;

    // binds the CSM texture arrays starting at GL_TEXTURE7
    void BindCSMTextures() const;

    // binds the point shadow cube array at GL_TEXTURE15
    void BindPointShadowTexture() const;

    [[nodiscard]] const std::vector<CascadedShadowMap*>&       GetCascadedShadowMaps()        const { return m_cascadedShadowMaps; }
    [[nodiscard]] const std::vector<ShadowedPointLightDebug>&  GetShadowedPointLightsDebug()  const { return m_shadowedPointLightsDebug; }
    [[nodiscard]] uint32_t GetShadowDrawCallCount() const { return m_shadowDrawCallCount; }
    [[nodiscard]] uint32_t GetShadowedPointLightCount() const { return static_cast<uint32_t>(m_shadowedPointLightsDebug.size()); }

    // reset per-frame stats (called by RenderApi at the start of Flush())
    void ResetStats() { m_shadowDrawCallCount = 0; }

private:
    struct ShadowCandidate {
        uint32_t index;
        float    score;
    };

    void EnsurePointShadowMetaBuffer(size_t pointLightCount);
    static float ScorePointLight(const PointLight* light, const Camera& camera);
    static void BuildPointShadowFaceMatrices(const glm::vec3& lightPos, float nearPlane, float farPlane, glm::mat4 outVP[6]);

    // directional shadow resources
    std::unique_ptr<Shader>              m_shadowDepthShader;
    std::vector<DirectionalLight*>       m_directionalLights;
    std::vector<CascadedShadowMap*>      m_cascadedShadowMaps;   // parallel to m_directionalLights

    // point shadow resources
    std::unique_ptr<Shader>              m_pointShadowDepthShader;
    std::unique_ptr<PointLightShadowMap> m_pointShadowMap;
    std::unique_ptr<ShaderStorageBuffer> m_pointShadowMetaSsbo;  // binding 8

    uint32_t m_shadowDrawCallCount = 0;
    std::vector<ShadowedPointLightDebug> m_shadowedPointLightsDebug;
};


#endif //MANGORENDERING_SHADOWRENDERER_H