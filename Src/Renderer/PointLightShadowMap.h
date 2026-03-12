
#ifndef MANGORENDERING_POINTLIGHTSHADOWMAP_H
#define MANGORENDERING_POINTLIGHTSHADOWMAP_H

#include <memory>
#include "Buffers/Framebuffer.h"

class PointLightShadowMap {
public:
    PointLightShadowMap(uint32_t resolution, uint32_t maxShadowedPointLights);
    ~PointLightShadowMap() = default;

    void BeginFace(uint32_t lightSlot, uint32_t face) const;
    void BeginLight(uint32_t lightSlot) const;
    static void End();

    [[nodiscard]] uint32_t GetTexture() const { return m_fb->GetDepthAttachment(); }
    [[nodiscard]] uint32_t GetMaxLights() const { return m_maxLights; }
    [[nodiscard]] uint32_t GetResolution() const { return m_resolution; }

private:
    std::unique_ptr<Framebuffer> m_fb;
    uint32_t m_resolution = 0;
    uint32_t m_maxLights  = 0;
};


#endif //MANGORENDERING_POINTLIGHTSHADOWMAP_H