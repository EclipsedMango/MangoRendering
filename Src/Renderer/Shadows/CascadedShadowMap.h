
#ifndef MANGORENDERING_CASCADEDSHADOWMAP_H
#define MANGORENDERING_CASCADEDSHADOWMAP_H

#include <array>
#include <memory>

#include "Renderer/Buffers/Framebuffer.h"
#include "Nodes/CameraNode3d.h"

class CascadedShadowMap {
public:
    static constexpr int NUM_CASCADES = 4;

    static constexpr std::array<uint32_t, NUM_CASCADES> CASCADE_RESOLUTIONS = { 8192, 2048, 1024, 512 };

    explicit CascadedShadowMap(const glm::vec3& lightDirection);
    ~CascadedShadowMap() = default;

    // refit all cascade frusta to the camera, call once per frame before the shadow pass.
    void Update(const CameraNode3d& camera);

    void BeginRender(int index) const;
    static void EndRender();

    void SetDirection(const glm::vec3& dir) { m_lightDirection = glm::normalize(dir); }

    [[nodiscard]] glm::mat4 GetLightSpaceMatrix(const int index) const { return m_lightSpaceMatrices[index]; }
    [[nodiscard]] uint32_t GetCascadeTexture(const int index) const { return m_cascadeFbs[index]->GetDepthAttachment(); }
    [[nodiscard]] const std::array<float, NUM_CASCADES>& GetSplitDistances() const { return m_splitDistances; }
    [[nodiscard]] float GetWorldUnitsPerTexel(const int i) const { return m_worldUnitsPerTexel[i]; }

private:
    std::array<std::unique_ptr<Framebuffer>, NUM_CASCADES> m_cascadeFbs;

    glm::vec3 m_lightDirection {};
    float m_lambda = 0.85f;

    std::array<glm::mat4, NUM_CASCADES> m_lightSpaceMatrices{};
    std::array<float, NUM_CASCADES> m_splitDistances{};
    std::array<float, NUM_CASCADES> m_worldUnitsPerTexel{};
};


#endif //MANGORENDERING_CASCADEDSHADOWMAP_H