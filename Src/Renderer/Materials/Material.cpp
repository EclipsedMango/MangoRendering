
#include "Material.h"

#include "Core/ResourceManager.h"

REGISTER_PROPERTY_TYPE(Material)

Material::Material() {
    AddProperty("albedo_color",
        [this]() -> PropertyValue { return glm::vec3(GetAlbedoColor()); },
        [this](const PropertyValue& v) { SetAlbedoColor(glm::vec4(std::get<glm::vec3>(v), m_albedoColor.a)); }
    );
    AddProperty("diffuse",
        [this]() -> PropertyValue { return m_diffuse ? m_diffuse->GetPath() : std::string(""); },
        [this](const PropertyValue& v) { SetDiffuse(std::get<std::string>(v)); }
    );

    AddProperty("metallic_value",
        [this]() -> PropertyValue { return GetMetallicValue(); },
        [this](const PropertyValue& v) { SetMetallicValue(std::get<float>(v)); }
    );
    AddProperty("metallic",
        [this]() -> PropertyValue { return m_metallic ? m_metallic->GetPath() : std::string(""); },
        [this](const PropertyValue& v) { SetMetallic(std::get<std::string>(v)); }
    );

    AddProperty("roughness_value",
        [this]() -> PropertyValue { return GetRoughnessValue(); },
        [this](const PropertyValue& v) { SetRoughnessValue(std::get<float>(v)); }
    );
    AddProperty("roughness",
        [this]() -> PropertyValue { return m_roughness ? m_roughness->GetPath() : std::string(""); },
        [this](const PropertyValue& v) { SetRoughness(std::get<std::string>(v)); }
    );

    AddProperty("ao_strength_value",
        [this]() -> PropertyValue { return GetAOStrength(); },
        [this](const PropertyValue& v) { SetAOStrength(std::get<float>(v)); }
    );
    AddProperty("ambient_occlusion",
        [this]() -> PropertyValue { return m_ambientOcclusion ? m_ambientOcclusion->GetPath() : std::string(""); },
        [this](const PropertyValue& v) { SetAmbientOcclusion(std::get<std::string>(v)); }
    );

    AddProperty("displacement_scale_value",
        [this]() -> PropertyValue { return GetDisplacementScale(); },
        [this](const PropertyValue& v) { SetDisplacementScale(std::get<float>(v)); }
    );
    AddProperty("displacement",
        [this]() -> PropertyValue { return m_displacement ? m_displacement->GetPath() : std::string(""); },
        [this](const PropertyValue& v) { SetDisplacement(std::get<std::string>(v)); }
    );

    AddProperty("normal_strength",
        [this]() -> PropertyValue { return GetNormalStrength(); },
        [this](const PropertyValue& v) { SetNormalStrength(std::get<float>(v)); }
    );
    AddProperty("normal",
        [this]() -> PropertyValue { return m_normal ? m_normal->GetPath() : std::string(""); },
        [this](const PropertyValue& v) { SetNormal(std::get<std::string>(v)); }
    );

    AddProperty("emission_strength",
        [this]() -> PropertyValue { return GetEmissionStrength(); },
        [this](const PropertyValue& v) { SetEmissionStrength(std::get<float>(v)); }
    );
    AddProperty("emissive",
        [this]() -> PropertyValue { return m_emissive ? m_emissive->GetPath() : std::string(""); },
        [this](const PropertyValue& v) { SetEmissive(std::get<std::string>(v)); }
    );

    AddProperty("double_sided",
        [this]() -> PropertyValue { return GetDoubleSided(); },
        [this](const PropertyValue& v) { SetDoubleSided(std::get<bool>(v)); }
    );
    AddProperty("cast_shadows",
        [this]() -> PropertyValue { return m_castShadows; },
        [this](const PropertyValue& v) { SetCastShadows(std::get<bool>(v)); }
    );
}

/*
 * Material textures layout order:
 *
 * diffuse / albedo == slot 0
 * normal == slot 1
 * metallic == slot 2
 * roughness == slot 3
 * ao == slot 4
 * emissive == slot 5
 * displacement == slot 6
 *
 */
void Material::Bind(const Shader &shader) const {
    shader.SetVector4("u_AlbedoColor", m_albedoColor);
    shader.SetFloat("u_MetallicValue", m_metallicValue);
    shader.SetFloat("u_RoughnessValue", m_roughnessValue);
    shader.SetFloat("u_AOStrength", m_aoStrength);
    shader.SetFloat("u_NormalStrength", m_normalStrength);
    shader.SetFloat("u_EmissionStrength", m_emissionStrength);
    shader.SetVector3("u_EmissionColor", m_emissionColor);
    shader.SetFloat("u_DisplacementScale", m_displacementScale);
    shader.SetBool("u_AlphaScissor", m_blendMode == BlendMode::AlphaScissor);
    shader.SetFloat("u_AlphaScissorThreshold", m_alphaScissorThreshold);
    shader.SetVector2("u_UVScale", m_uvScale);
    shader.SetVector2("u_UVOffset", m_uvOffset);
    shader.SetBool("u_DoubleSided", m_doubleSided);

    const bool packed = m_metallic && m_roughness && m_metallic == m_roughness;
    shader.SetBool("u_HasMetallicRoughnessPacked", packed);

    if (m_diffuse) {
        m_diffuse->Bind(0);
        shader.SetInt("u_Diffuse", 0);
        shader.SetBool("u_HasDiffuse", true);
    } else {
        shader.SetBool("u_HasDiffuse", false);
    }

    if (m_normal) {
        m_normal->Bind(1);
        shader.SetInt("u_Normal", 1);
        shader.SetBool("u_HasNormal", true);
    } else {
        shader.SetBool("u_HasNormal", false);
    }


    if (m_metallic) {
        m_metallic->Bind(2);
        shader.SetInt("u_Metallic", 2);
        shader.SetBool("u_HasMetallic", !packed);
    } else {
        shader.SetBool("u_HasMetallic", false);
    }

    if (!packed && m_roughness) {
        m_roughness->Bind(3);
        shader.SetInt("u_Roughness", 3);
        shader.SetBool("u_HasRoughness", true);
    } else {
        shader.SetBool("u_HasRoughness", false);
    }

    if (m_ambientOcclusion) {
        m_ambientOcclusion->Bind(4);
        shader.SetInt("u_AmbientOcclusion", 4);
        shader.SetBool("u_HasAmbientOcclusion", true);
    } else {
        shader.SetBool("u_HasAmbientOcclusion", false);
    }

    if (m_emissive) {
        m_emissive->Bind(5);
        shader.SetInt("u_Emissive", 5);
        shader.SetBool("u_HasEmissive", true);
    } else {
        shader.SetBool("u_HasEmissive", false);
    }

    if (m_displacement) {
        m_displacement->Bind(6);
        shader.SetInt("u_Displacement", 6);
        shader.SetBool("u_HasDisplacement", true);
    } else {
        shader.SetBool("u_HasDisplacement", false);
    }
}

void Material::SetName(const std::string &name) {
    m_name = name;
    m_dirty = true;
}

void Material::SetAlbedoColor(const glm::vec4 &color) {
    m_albedoColor = color;
    m_dirty = true;
}

void Material::SetMetallicValue(const float value) {
    m_metallicValue = value;
    m_dirty = true;
}

void Material::SetRoughnessValue(const float value) {
    m_roughnessValue = value;
    m_dirty = true;
}

void Material::SetAOStrength(const float value) {
    m_aoStrength = value;
    m_dirty = true;
}

void Material::SetNormalStrength(const float value) {
    m_normalStrength = value;
    m_dirty = true;
}

void Material::SetEmissionStrength(const float value) {
    m_emissionStrength = value;
    m_dirty = true;
}

void Material::SetEmissionColor(const glm::vec3 &color) {
    m_emissionColor = color;
    m_dirty = true;
}

void Material::SetDisplacementScale(const float value) {
    m_displacementScale = value;
    m_dirty = true;
}

void Material::SetUseDisplacement(const bool value) {
    m_useDisplacement = value;
    m_dirty = true;
}

void Material::SetCastShadows(const bool value) {
    m_castShadows = value;
    m_dirty = true;
}

void Material::SetDoubleSided(const bool value) {
    m_doubleSided = value;
    m_dirty = true;
}

void Material::SetDirty(const bool value) {
    m_dirty = value;
}

void Material::SetBlendMode(const BlendMode mode) {
    m_blendMode = mode;
    m_dirty = true;
}

void Material::SetAlphaScissorThreshold(const float value) {
    m_alphaScissorThreshold = value;
    m_dirty = true;
}

void Material::SetUVScale(const glm::vec2 &scale) {
    m_uvScale = scale;
    m_dirty = true;
}

void Material::SetUVOffset(const glm::vec2 &offset) {
    m_uvOffset = offset;
    m_dirty = true;
}

void Material::SetDiffuse(const std::string &path) {
    m_diffuse = ResourceManager::Get().LoadTexture(path);
    m_dirty = true;
}

void Material::SetAmbientOcclusion(const std::string &path) {
    m_ambientOcclusion = ResourceManager::Get().LoadTexture(path);
    m_dirty = true;
}

void Material::SetNormal(const std::string &path) {
    m_normal = ResourceManager::Get().LoadTexture(path);
    m_dirty = true;
}

void Material::SetRoughness(const std::string &path) {
    m_roughness = ResourceManager::Get().LoadTexture(path);
    m_dirty = true;
}

void Material::SetMetallic(const std::string &path) {
    m_metallic = ResourceManager::Get().LoadTexture(path);
    m_dirty = true;
}

void Material::SetDisplacement(const std::string &path) {
    m_displacement = ResourceManager::Get().LoadTexture(path);
    m_dirty = true;
}

void Material::SetEmissive(const std::string &path) {
    m_emissive = ResourceManager::Get().LoadTexture(path);
    m_dirty = true;
}
