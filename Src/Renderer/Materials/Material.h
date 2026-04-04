
#ifndef MANGORENDERING_MATERIAL_H
#define MANGORENDERING_MATERIAL_H
#include <memory>

#include "Texture.h"
#include <glm/glm.hpp>

#include "Core/PropertyHolder.h"
#include "Renderer/Shader.h"

enum class BlendMode { Opaque, AlphaBlend, AlphaScissor, Additive };

class Material : public PropertyHolder {
public:
    Material();

    void Bind(const Shader& shader) const;

    void SetName(const std::string& name);
    void SetFilePath(const std::string& path);
    void SetAlbedoColor(const glm::vec4& color);
    void SetMetallicValue(float value);
    void SetRoughnessValue(float value);
    void SetAOStrength(float value);
    void SetNormalStrength(float value);
    void SetEmissionStrength(float value);
    void SetEmissionColor(const glm::vec3& color);
    void SetDisplacementScale(float value);
    void SetUseDisplacement(bool value);
    void SetCastShadows(bool value);
    void SetDoubleSided(bool value);
    void SetDirty(bool value);
    void SetBlendMode(BlendMode mode);
    void SetAlphaScissorThreshold(float value);
    void SetUVScale(const glm::vec2& scale);
    void SetUVOffset(const glm::vec2& offset);

    [[nodiscard]] std::string GetPropertyHolderType() const override { return "Material"; }

    [[nodiscard]] std::string GetName() const { return m_name; }
    [[nodiscard]] std::string GetFilePath() const { return m_filePath; }
    [[nodiscard]] glm::vec4 GetAlbedoColor() const { return m_albedoColor; }
    [[nodiscard]] float GetMetallicValue() const { return m_metallicValue; }
    [[nodiscard]] float GetRoughnessValue() const { return m_roughnessValue; }
    [[nodiscard]] float GetAOStrength() const { return m_aoStrength; }
    [[nodiscard]] float GetNormalStrength() const { return m_normalStrength; }
    [[nodiscard]] float GetEmissionStrength() const { return m_emissionStrength; }
    [[nodiscard]] glm::vec3 GetEmissionColor() const { return m_emissionColor; }
    [[nodiscard]] float GetDisplacementScale() const { return m_displacementScale; }
    [[nodiscard]] bool GetUseDisplacement() const { return m_useDisplacement; }
    [[nodiscard]] bool GetCastShadows() const { return m_castShadows; }
    [[nodiscard]] bool GetDoubleSided() const { return m_doubleSided; }
    [[nodiscard]] bool GetIsDirty() const { return m_dirty; }
    [[nodiscard]] BlendMode GetBlendMode() const { return m_blendMode; }
    [[nodiscard]] float GetAlphaScissorThreshold() const { return m_alphaScissorThreshold; }
    [[nodiscard]] glm::vec2 GetUVScale() const { return m_uvScale; }
    [[nodiscard]] glm::vec2 GetUVOffset() const { return m_uvOffset; }

    void SetDiffuse(const std::string& path);
    void SetAmbientOcclusion(const std::string& path);
    void SetNormal(const std::string& path);
    void SetRoughness(const std::string& path);
    void SetMetallic(const std::string& path);
    void SetDisplacement(const std::string& path);
    void SetEmissive(const std::string& path);

    [[nodiscard]] std::shared_ptr<Texture> GetDiffuse() const { return m_diffuse; }
    [[nodiscard]] std::shared_ptr<Texture> GetAmbientOcclusion() const { return m_ambientOcclusion; }
    [[nodiscard]] std::shared_ptr<Texture> GetNormal() const { return m_normal; }
    [[nodiscard]] std::shared_ptr<Texture> GetRoughness() const { return m_roughness; }
    [[nodiscard]] std::shared_ptr<Texture> GetMetallic() const { return m_metallic; }
    [[nodiscard]] std::shared_ptr<Texture> GetDisplacement() const { return m_displacement; }
    [[nodiscard]] std::shared_ptr<Texture> GetEmissive() const { return m_emissive; }

private:
    std::string m_name;
    std::string m_filePath;

    glm::vec4 m_albedoColor = glm::vec4(1.0f);
    float m_metallicValue = 0.0f;
    float m_roughnessValue = 1.0f;
    float m_aoStrength = 1.0f;
    float m_normalStrength = 1.0f;
    float m_emissionStrength = 1.0f;
    glm::vec3 m_emissionColor = glm::vec3(0.0f);

    float m_displacementScale = 0.01f;

    bool m_useDisplacement = false;
    bool m_castShadows = true;
    bool m_doubleSided = false;
    bool m_dirty = false;

    BlendMode m_blendMode = BlendMode::Opaque;
    float m_alphaScissorThreshold = 0.5f;

    glm::vec2 m_uvScale = glm::vec2(1.0f);
    glm::vec2 m_uvOffset = glm::vec2(0.0f);

    std::shared_ptr<Texture> m_diffuse {};
    std::shared_ptr<Texture> m_ambientOcclusion {};
    std::shared_ptr<Texture> m_normal {};
    std::shared_ptr<Texture> m_roughness {};
    std::shared_ptr<Texture> m_metallic {};
    std::shared_ptr<Texture> m_displacement {};
    std::shared_ptr<Texture> m_emissive {};

    // TODO: add sss, for future because this is hard
    // Texture m_subsurface;
};


#endif //MANGORENDERING_MATERIAL_H