
#include "PointLightShadowMap.h"

#include "glad/gl.h"

PointLightShadowMap::PointLightShadowMap(const uint32_t resolution, const uint32_t maxShadowedPointLights) : m_resolution(resolution), m_maxLights(maxShadowedPointLights) {
    m_fb = std::make_unique<Framebuffer>(m_resolution, m_resolution, FramebufferType::DepthCubeArray, m_maxLights * 6);
}

void PointLightShadowMap::BeginFace(const uint32_t lightSlot, const uint32_t face) const {
    const uint32_t layer = lightSlot * 6u + face;
    m_fb->BindLayer(layer);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void PointLightShadowMap::BeginLight(const uint32_t lightSlot) const {
    glBindFramebuffer(GL_FRAMEBUFFER, m_fb->GetFbo());
    glViewport(0, 0, m_resolution, m_resolution);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    for (uint32_t face = 0; face < 6; face++) {
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_fb->GetDepthAttachment(), 0, lightSlot * 6u + face);
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_fb->GetDepthAttachment(), 0);
}

void PointLightShadowMap::End() {
    Framebuffer::Unbind();
}
