#include "CascadedShadowMap.h"

#include <limits>
#include <vector>
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>

CascadedShadowMap::CascadedShadowMap(const glm::vec3& lightDirection) : m_lightDirection(glm::normalize(lightDirection)) {
    for (int i = 0; i < NUM_CASCADES; i++) {
        const uint32_t res = CASCADE_RESOLUTIONS[i];
        m_cascadeFbs[i] = std::make_unique<Framebuffer>(res, res, FramebufferType::DepthOnly);
    }
}

void CascadedShadowMap::Update(const CameraNode3d& camera) {
    const float near = camera.GetNearPlane();
    const float far = camera.GetFarPlane();

    std::array<float, NUM_CASCADES + 1> splits {};
    splits[0] = near;
    for (int i = 1; i < NUM_CASCADES; i++) {
        const float ratio = static_cast<float>(i) / static_cast<float>(NUM_CASCADES);
        const float logSplit = near * powf(far / near, ratio);
        const float linSplit = near + (far - near) * ratio;
        splits[i] = glm::mix(logSplit, linSplit, m_lambda);
    }
    splits[NUM_CASCADES] = far;

    for (int i = 0; i < NUM_CASCADES; i++) {
        m_splitDistances[i] = splits[i + 1];
    }

    for (int i = 0; i < NUM_CASCADES; i++) {
        const glm::mat4 inv = glm::inverse(
            glm::perspective(camera.GetFov(), camera.GetAspectRatio(), splits[i], splits[i + 1]) * camera.GetViewMatrix()
        );

        glm::vec4 corners[8];
        int idx = 0;
        for (int x = 0; x < 2; x++)
        for (int y = 0; y < 2; y++)
        for (int z = 0; z < 2; z++) {
            const glm::vec4 pt = inv * glm::vec4(2.0f*x-1.0f, 2.0f*y-1.0f, 2.0f*z-1.0f, 1.0f);
            corners[idx++] = pt / pt.w;
        }

        glm::vec3 center = glm::vec3(0.0f);
        for (const auto& c : corners) {
            center += glm::vec3(c);
        }
        center /= 8.0f;

        float radius = 0.0f;
        for (const auto& c : corners) {
            radius = std::max(radius, glm::length(glm::vec3(c) - center));
        }

        const float res = static_cast<float>(CASCADE_RESOLUTIONS[i]);
        const float worldUnitsPerTexel = 2.0f * radius / res;
        m_worldUnitsPerTexel[i] = worldUnitsPerTexel;

        glm::vec3 up(0,1,0);
        if (std::abs(glm::dot(m_lightDirection, up)) > 0.99f) {
            up = glm::vec3(0,0,1);
        }

        glm::mat4 lightViewRot = glm::lookAt(glm::vec3(0.0f), m_lightDirection, up);
        glm::vec3 centerLS = glm::vec3(lightViewRot * glm::vec4(center, 1.0f));
        centerLS.x = std::floor(centerLS.x / worldUnitsPerTexel) * worldUnitsPerTexel;
        centerLS.y = std::floor(centerLS.y / worldUnitsPerTexel) * worldUnitsPerTexel;

        glm::mat4 lightView = glm::translate(glm::mat4(1.0f), -centerLS) * lightViewRot;

        float minZ =  std::numeric_limits<float>::max();
        float maxZ = -std::numeric_limits<float>::max();
        for (const auto& c : corners) {
            float z = (lightView * c).z;
            minZ = std::min(minZ, z);
            maxZ = std::max(maxZ, z);
        }

        constexpr float zMult = 1.5f;
        if (minZ < 0) minZ *= zMult; else minZ /= zMult;
        if (maxZ < 0) maxZ /= zMult; else maxZ *= zMult;

        m_lightSpaceMatrices[i] = glm::ortho(-radius, radius, -radius, radius, minZ, maxZ) * lightView;
    }
}

void CascadedShadowMap::BeginRender(const int index) const {
    m_cascadeFbs[index]->Bind();
    const uint32_t res = CASCADE_RESOLUTIONS[index];
    glViewport(0, 0, static_cast<GLsizei>(res), static_cast<GLsizei>(res));
    glClear(GL_DEPTH_BUFFER_BIT);
}

void CascadedShadowMap::EndRender() {
    Framebuffer::Unbind();
}
