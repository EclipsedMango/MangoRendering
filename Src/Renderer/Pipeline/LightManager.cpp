
#include "LightManager.h"

#include <algorithm>
#include <stdexcept>

#include "../Lights/GpuLights.h"

void LightManager::AddDirectionalLight(DirectionalLight* light) {
    if (!light) throw std::runtime_error("LightManager: null directional light");
    if (m_directionalLights.size() >= MAX_DIR_LIGHTS) throw std::runtime_error("LightManager: max directional lights reached");
    m_directionalLights.push_back(light);
}

void LightManager::RemoveDirectionalLight(DirectionalLight* light) {
    std::erase(m_directionalLights, light);
}

void LightManager::AddPointLight(PointLight* light) {
    if (light) m_pointLights.push_back(light);
}

void LightManager::RemovePointLight(PointLight* light) {
    std::erase(m_pointLights, light);
}

void LightManager::AddSpotLight(SpotLight* light) {
    if (light) m_spotLights.push_back(light);
}

void LightManager::RemoveSpotLight(SpotLight* light) {
    std::erase(m_spotLights, light);
}

void LightManager::Upload() {
    // --- directional lights + counts UBO (binding 1) ---
    constexpr size_t uboSize = sizeof(glm::ivec4) + MAX_DIR_LIGHTS * 2 * sizeof(glm::vec4);
    if (!m_globalLightUbo) {
        m_globalLightUbo = std::make_unique<UniformBuffer>(uboSize, 1);
    }

    const glm::ivec4 counts(
        static_cast<int>(m_directionalLights.size()),
        static_cast<int>(m_pointLights.size()),
        static_cast<int>(m_spotLights.size()),
        0
    );
    m_globalLightUbo->SetData(&counts, sizeof(glm::ivec4), 0);

    size_t offset = sizeof(glm::ivec4);
    for (size_t i = 0; i < MAX_DIR_LIGHTS; i++) {
        GPUDirectionalLight gpuLight{};
        if (i < m_directionalLights.size()) {
            const DirectionalLight* l = m_directionalLights[i];
            gpuLight.direction = glm::vec4(l->GetDirection(), 0.0f);
            gpuLight.color = glm::vec4(l->GetColor(), l->GetIntensity());
        }

        m_globalLightUbo->SetData(&gpuLight, sizeof(GPUDirectionalLight), offset);
        offset += sizeof(GPUDirectionalLight);
    }

    // --- point lights SSBO (binding 2) ---
    if (!m_pointLights.empty()) {
        std::vector<GPUPointLight> pointData;
        pointData.reserve(m_pointLights.size());

        for (const auto* l : m_pointLights) {
            GPUPointLight gl{};
            gl.position = glm::vec4(l->GetPosition(), l->GetRadius());
            gl.color = glm::vec4(l->GetColor(), l->GetIntensity());
            gl.attenuation = glm::vec4(l->GetConstant(), l->GetLinear(), l->GetQuadratic(), 0.0f);
            pointData.push_back(gl);
        }

        const size_t size = pointData.size() * sizeof(GPUPointLight);
        if (!m_pointLightSsbo || m_pointLightSsbo->GetSize() < size) {
            m_pointLightSsbo = std::make_unique<ShaderStorageBuffer>(size, 2);
        }

        m_pointLightSsbo->SetData(pointData.data(), size, 0);
    }

    // --- spot lights SSBO (binding 3) ---
    if (!m_spotLights.empty()) {
        std::vector<GPUSpotLight> spotData;
        spotData.reserve(m_spotLights.size());

        for (const auto* l : m_spotLights) {
            GPUSpotLight gl{};
            const float radius = CalculateLightRadius(l->GetColor(), l->GetIntensity(), 1.0f, l->GetLinear(), l->GetQuadratic());
            gl.position  = glm::vec4(l->GetPosition(), radius);
            gl.direction = glm::vec4(l->GetDirection(), 0.0f);
            gl.color = glm::vec4(l->GetColor(), l->GetIntensity());
            gl.params = glm::vec4(l->GetCutOff(), l->GetOuterCutOff(), l->GetLinear(), l->GetQuadratic());
            spotData.push_back(gl);
        }

        const size_t size = spotData.size() * sizeof(GPUSpotLight);
        if (!m_spotLightSsbo || m_spotLightSsbo->GetSize() < size) {
            m_spotLightSsbo = std::make_unique<ShaderStorageBuffer>(size, 3);
        }

        m_spotLightSsbo->SetData(spotData.data(), size, 0);
    }
}

float LightManager::CalculateLightRadius(const glm::vec3& color, const float intensity, const float constant, const float linear, const float quadratic) {
    constexpr float threshold = 0.05f;

    const float maxBrightness = std::max({ color.r, color.g, color.b }) * intensity;
    if (maxBrightness <= 0.0f) {
        return 0.0f;
    }

    const float a = quadratic;
    const float b = linear;
    const float c = constant - maxBrightness / threshold;

    if (std::abs(a) < 1e-6f) {
        if (std::abs(b) < 1e-6f) {
            return 1000.0f;
        }

        return std::max(0.0f, -c / b);
    }

    const float disc = b * b - 4.0f * a * c;
    if (disc < 0.0f) {
        return 0.0f;
    }

    return std::max(0.0f, (-b + std::sqrt(disc)) / (2.0f * a));
}