
#ifndef MANGORENDERING_MATERIAL_H
#define MANGORENDERING_MATERIAL_H
#include <memory>

#include "Texture.h"
#include <glm/glm.hpp>

#include "Shader.h"

enum class BlendMode { Opaque, AlphaBlend, AlphaScissor, Additive };

class Material {
public:
    void Bind(const Shader& shader) const;

    void SetName(const std::string& name) {
        m_name = name;
    }
    void SetAlbedoColor(const glm::vec4& color) {
        m_albedoColor = color;
        m_dirty = true;
    }
    void SetMetallicValue(const float value) {
        m_metallicValue = value;
        m_dirty = true;
    }
    void SetRoughnessValue(const float value) {
        m_roughnessValue = value;
        m_dirty = true;
    }
    void SetAOStrength(const float value) {
        m_aoStrength = value;
        m_dirty = true;
    }
    void SetNormalStrength(const float value) {
        m_normalStrength = value;
        m_dirty = true;
    }
    void SetEmissionStrength(const float value) {
        m_emissionStrength = value;
        m_dirty = true;
    }
    void SetEmissionColor(const glm::vec3& color) {
        m_emissionColor = color;
        m_dirty = true;
    }
    void SetDisplacementScale(const float value) {
        m_displacementScale = value;
        m_dirty = true;
    }
    void SetUseDisplacement(const bool value) {
        m_useDisplacement = value;
        m_dirty = true;
    }
    void SetCastShadows(const bool value) {
        m_castShadows = value;
        m_dirty = true;
    }
    void SetDoubleSided(const bool value) {
        m_doubleSided = value;
        m_dirty = true;
    }
    void SetDirty(const bool value) {
        m_dirty = value;
    }
    void SetBlendMode(const BlendMode mode) {
        m_blendMode = mode;
        m_dirty = true;
    }
    void SetAlphaScissorThreshold(const float value) {
        m_alphaScissorThreshold = value;
        m_dirty = true;
    }
    void SetUVScale(const glm::vec2& scale) {
        m_uvScale = scale;
        m_dirty = true;
    }
    void SetUVOffset(const glm::vec2& offset) {
        m_uvOffset = offset;
        m_dirty = true;
    }

    [[nodiscard]] std::string GetName() const { return m_name; }
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

    void SetDiffuse(const std::shared_ptr<Texture> &texture) {
        m_diffuse = texture;
        m_dirty = true;
    }
    void SetAmbientOcclusion(const std::shared_ptr<Texture> &texture) {
        m_ambientOcclusion = texture;
        m_dirty = true;
    }
    void SetNormal(const std::shared_ptr<Texture> &texture) {
        m_normal = texture;
        m_dirty = true;
    }
    void SetRoughness(const std::shared_ptr<Texture> &texture) {
        m_roughness = texture;
        m_dirty = true;
    }
    void SetMetallic(const std::shared_ptr<Texture> &texture) {
        m_metallic = texture;
        m_dirty = true;
    }
    void SetDisplacement(const std::shared_ptr<Texture> &texture) {
        m_displacement = texture;
        m_dirty = true;
    }
    void SetEmissive(const std::shared_ptr<Texture> &texture) {
        m_emissive = texture;
        m_dirty = true;
    }

    [[nodiscard]] std::shared_ptr<Texture> GetDiffuse() const { return m_diffuse; }
    [[nodiscard]] std::shared_ptr<Texture> GetAmbientOcclusion() const { return m_ambientOcclusion; }
    [[nodiscard]] std::shared_ptr<Texture> GetNormal() const { return m_normal; }
    [[nodiscard]] std::shared_ptr<Texture> GetRoughness() const { return m_roughness; }
    [[nodiscard]] std::shared_ptr<Texture> GetMetallic() const { return m_metallic; }
    [[nodiscard]] std::shared_ptr<Texture> GetDisplacement() const { return m_displacement; }
    [[nodiscard]] std::shared_ptr<Texture> GetEmissive() const { return m_emissive; }

private:
    std::string m_name;

    glm::vec4 m_albedoColor = glm::vec4(1.0f);
    float m_metallicValue    = 0.0f; // used when no metallic tex bound
    float m_roughnessValue   = 0.5f;
    float m_aoStrength       = 1.0f;
    float m_normalStrength   = 1.0f;
    float m_emissionStrength = 1.0f;
    glm::vec3 m_emissionColor = glm::vec3(0.0f);

    float m_displacementScale = 0.05f;

    bool m_useDisplacement  = false;
    bool m_castShadows      = true;
    bool m_doubleSided      = false;
    bool m_dirty            = false;

    BlendMode m_blendMode = BlendMode::Opaque;
    float m_alphaScissorThreshold = 0.5f;

    glm::vec2 m_uvScale  = glm::vec2(1.0f);
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