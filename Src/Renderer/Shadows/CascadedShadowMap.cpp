#include "CascadedShadowMap.h"

#include <vector>
#include <Glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

CascadedShadowMap::CascadedShadowMap(const glm::vec3& lightDirection) : m_lightDirection(glm::normalize(lightDirection)) {
    for (int i = 0; i < NUM_CASCADES; i++) {
        m_cascadeFbs[i] = std::make_unique<Framebuffer>(static_cast<uint32_t>(CASCADE_RESOLUTION), static_cast<uint32_t>(CASCADE_RESOLUTION), FramebufferType::DepthOnly);
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
        splits[i] = glm::mix(linSplit, logSplit, m_lambda);
    }
    splits[NUM_CASCADES] = far;

    for (int i = 0; i < NUM_CASCADES; i++) {
        m_splitNearDistances[i] = splits[i];
        m_splitDistances[i] = splits[i + 1];
    }

    glm::vec3 up(0.0f, 1.0f, 0.0f);
    if (std::abs(glm::dot(m_lightDirection, up)) > 0.999f) {
        up = glm::vec3(0.0f, 0.0f, 1.0f);
    }

    for (int i = 0; i < NUM_CASCADES; i++) {
        const glm::mat4 inv = glm::inverse(
            glm::perspective(camera.GetFov(), camera.GetAspectRatio(), splits[i], splits[i + 1]) * camera.GetViewMatrix()
        );

        glm::vec4 corners[8];
        int idx = 0;
        for (int x = 0; x < 2; x++) {
            for (int y = 0; y < 2; y++) {
                for (int z = 0; z < 2; z++) {
                    const glm::vec4 pt = inv * glm::vec4(2.0f*x-1.0f, 2.0f*y-1.0f, 2.0f*z-1.0f, 1.0f);
                    corners[idx++] = pt / pt.w;
                }
            }
        }

        glm::vec3 center = glm::vec3(0.0f);
        for (const auto& c : corners) {
            center += glm::vec3(c);
        }

        glm::mat4 lightView = glm::lookAt(glm::vec3(0.0f), m_lightDirection, up);

        float minX = std::numeric_limits<float>::max();
        float maxX = -std::numeric_limits<float>::max();
        float minY = std::numeric_limits<float>::max();
        float maxY = -std::numeric_limits<float>::max();
        float minZ = std::numeric_limits<float>::max();
        float maxZ = -std::numeric_limits<float>::max();

        for (const auto& c : corners) {
            glm::vec4 trf = lightView * c;
            minX = std::min(minX, trf.x);
            maxX = std::max(maxX, trf.x);
            minY = std::min(minY, trf.y);
            maxY = std::max(maxY, trf.y);
            minZ = std::min(minZ, trf.z);
            maxZ = std::max(maxZ, trf.z);
        }

        // minX -= m_offscreenCasterPadding;
        // maxX += m_offscreenCasterPadding;
        // minY -= m_offscreenCasterPadding;
        // maxY += m_offscreenCasterPadding;

        float worldWidth = maxX - minX;
        float worldHeight = maxY - minY;

        m_worldUnitsPerTexel[i] = std::max(worldWidth, worldHeight) / static_cast<float>(CASCADE_RESOLUTION);

        float texelSize = m_worldUnitsPerTexel[i];
        minX = std::floor(minX / texelSize) * texelSize;
        maxX = std::floor(maxX / texelSize) * texelSize;
        minY = std::floor(minY / texelSize) * texelSize;
        maxY = std::floor(maxY / texelSize) * texelSize;

        const float cascadeDepth = splits[i + 1] - splits[i];
        const float cascadeFootprint = glm::length(glm::vec2(worldWidth, worldHeight));

        // keep enough depth slack for off-camera casters that can still project into the visible cascade
        const float zPadding = glm::max(glm::max(m_depthCasterPadding, cascadeDepth), cascadeFootprint);

        float zNear = -maxZ - zPadding;
        float zFar = -minZ + zPadding;

        m_lightSpaceMatrices[i] = glm::ortho(minX, maxX, minY, maxY, zNear, zFar) * lightView;
    }
}

void CascadedShadowMap::BeginRender(const int index) const {
    m_cascadeFbs[index]->Bind();
    glViewport(0, 0, static_cast<GLsizei>(CASCADE_RESOLUTION), static_cast<GLsizei>(CASCADE_RESOLUTION));
    glClear(GL_DEPTH_BUFFER_BIT);
}

void CascadedShadowMap::EndRender() {
    Framebuffer::Unbind();
}
