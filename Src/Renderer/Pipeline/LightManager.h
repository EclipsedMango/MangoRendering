
#ifndef MANGORENDERING_LIGHTMANAGER_H
#define MANGORENDERING_LIGHTMANAGER_H

#include <memory>
#include <vector>

#include "glm/glm.hpp"
#include "Renderer/Lights/DirectionalLight.h"
#include "Renderer/Lights/PointLight.h"
#include "Renderer/Lights/SpotLight.h"
#include "Renderer/Buffers/UniformBuffer.h"
#include "Renderer/Buffers/ShaderStorageBuffer.h"

class LightManager {
public:
    static constexpr uint32_t MAX_DIR_LIGHTS = 4;

    LightManager() = default;
    ~LightManager() = default;

    LightManager(const LightManager&) = delete;
    LightManager& operator=(const LightManager&) = delete;
    LightManager(LightManager&&) = delete;
    LightManager& operator=(LightManager&&) = delete;

    void AddDirectionalLight(DirectionalLight* light);
    void RemoveDirectionalLight(DirectionalLight* light);

    void AddPointLight(PointLight* light);
    void RemovePointLight(PointLight* light);

    void AddSpotLight(SpotLight* light);
    void RemoveSpotLight(SpotLight* light);

    // uploads all light data to the GPU, called once per frame before drawing
    void Upload();

    [[nodiscard]] const std::vector<DirectionalLight*>& GetDirectionalLights() const { return m_directionalLights; }
    [[nodiscard]] const std::vector<PointLight*>& GetPointLights() const { return m_pointLights; }
    [[nodiscard]] const std::vector<SpotLight*>& GetSpotLights() const { return m_spotLights; }

    [[nodiscard]] uint32_t GetDirectionalLightCount() const { return static_cast<uint32_t>(m_directionalLights.size()); }
    [[nodiscard]] uint32_t GetPointLightCount() const { return static_cast<uint32_t>(m_pointLights.size()); }
    [[nodiscard]] uint32_t GetSpotLightCount() const { return static_cast<uint32_t>(m_spotLights.size()); }

    // exposed for the cluster culling pass in RenderApi
    [[nodiscard]] ShaderStorageBuffer* GetPointLightSsbo() const { return m_pointLightSsbo.get(); }
    [[nodiscard]] ShaderStorageBuffer* GetSpotLightSsbo() const { return m_spotLightSsbo.get(); }

private:
    static float CalculateLightRadius(const glm::vec3& color, float intensity, float constant, float linear, float quadratic);

    std::vector<DirectionalLight*> m_directionalLights;
    std::vector<PointLight*> m_pointLights;
    std::vector<SpotLight*> m_spotLights;

    std::unique_ptr<UniformBuffer> m_globalLightUbo;  // binding 1
    std::unique_ptr<ShaderStorageBuffer> m_pointLightSsbo;  // binding 2
    std::unique_ptr<ShaderStorageBuffer> m_spotLightSsbo;   // binding 3
};


#endif //MANGORENDERING_LIGHTMANAGER_H