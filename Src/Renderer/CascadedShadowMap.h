
#ifndef MANGORENDERING_CASCADEDSHADOWMAP_H
#define MANGORENDERING_CASCADEDSHADOWMAP_H

#include <array>
#include <memory>
#include <vector>

#include "Buffers/Framebuffer.h"
#include "Scene/Camera.h"

class CascadedShadowMap {
public:
    static constexpr int NUM_CASCADES = 4;

    CascadedShadowMap(uint32_t width, uint32_t height, const glm::vec3& lightDirection);
    ~CascadedShadowMap() = default;

    // refit all cascade frustum to the camera, call once per frame before the shadow pass.
    void Update(const Camera& camera);

    void BeginRender(int index) const;
    static void EndRender();

    [[nodiscard]] glm::mat4 GetLightSpaceMatrix(const int index) const { return m_lightSpaceMatrices[index]; }
    [[nodiscard]] uint32_t GetTextureArray() const { return m_fb->GetDepthAttachment(); }
    [[nodiscard]] const std::array<float, NUM_CASCADES>& GetSplitDistances() const { return m_splitDistances; }

private:
    std::unique_ptr<Framebuffer> m_fb;
    uint32_t m_width  = 0;
    uint32_t m_height = 0;

    glm::vec3 m_lightDirection {};
    float     m_lambda = 0.75f;

    std::array<glm::mat4, NUM_CASCADES> m_lightSpaceMatrices{};
    std::array<float,     NUM_CASCADES> m_splitDistances{};
};


#endif //MANGORENDERING_CASCADEDSHADOWMAP_H