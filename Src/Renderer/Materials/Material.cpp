
#include "Material.h"

Material::Material() {
    AddProperty("albedo_color",
        [this]() -> PropertyValue { return glm::vec3(GetAlbedoColor()); },
        [this](const PropertyValue& v) { SetAlbedoColor(glm::vec4(std::get<glm::vec3>(v), m_albedoColor.a)); }
    );
    AddProperty("diffuse",
        [this]() -> PropertyValue { return m_diffuse; },
        [this](const PropertyValue& v) { SetDiffuse(std::get<std::shared_ptr<Texture>>(v)); }
    );

    AddProperty("metallic_value",
        [this]() -> PropertyValue { return GetMetallicValue(); },
        [this](const PropertyValue& v) { SetMetallicValue(std::get<float>(v)); }
    );
    AddProperty("metallic",
        [this]() -> PropertyValue { return m_metallic; },
        [this](const PropertyValue& v) { SetMetallic(std::get<std::shared_ptr<Texture>>(v)); }
    );

    AddProperty("roughness_value",
        [this]() -> PropertyValue { return GetRoughnessValue(); },
        [this](const PropertyValue& v) { SetRoughnessValue(std::get<float>(v)); }
    );
    AddProperty("roughness",
        [this]() -> PropertyValue { return m_roughness; },
        [this](const PropertyValue& v) { SetRoughness(std::get<std::shared_ptr<Texture>>(v)); }
    );

    AddProperty("ao_strength_value",
        [this]() -> PropertyValue { return GetAOStrength(); },
        [this](const PropertyValue& v) { SetAOStrength(std::get<float>(v)); }
    );
    AddProperty("ambient_occlusion",
        [this]() -> PropertyValue { return m_ambientOcclusion; },
        [this](const PropertyValue& v) { SetAmbientOcclusion(std::get<std::shared_ptr<Texture>>(v)); }
    );

    AddProperty("displacement_scale_value",
        [this]() -> PropertyValue { return GetDisplacementScale(); },
        [this](const PropertyValue& v) { SetDisplacementScale(std::get<float>(v)); }
    );
    AddProperty("displacement",
        [this]() -> PropertyValue { return m_displacement; },
        [this](const PropertyValue& v) { SetDisplacement(std::get<std::shared_ptr<Texture>>(v)); }
    );

    AddProperty("normal_strength",
        [this]() -> PropertyValue { return GetNormalStrength(); },
        [this](const PropertyValue& v) { SetNormalStrength(std::get<float>(v)); }
    );
    AddProperty("normal",
        [this]() -> PropertyValue { return m_normal; },
        [this](const PropertyValue& v) { SetNormal(std::get<std::shared_ptr<Texture>>(v)); }
    );

    AddProperty("emission_strength",
        [this]() -> PropertyValue { return GetEmissionStrength(); },
        [this](const PropertyValue& v) { SetEmissionStrength(std::get<float>(v)); }
    );
    AddProperty("emissive",
        [this]() -> PropertyValue { return m_emissive; },
        [this](const PropertyValue& v) { SetEmissive(std::get<std::shared_ptr<Texture>>(v)); }
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
