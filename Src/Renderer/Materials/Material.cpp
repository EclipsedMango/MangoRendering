
#include "Material.h"

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
    shader.SetVector2("u_UVScale", m_uvScale);
    shader.SetVector2("u_UVOffset", m_uvOffset);

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
        shader.SetBool("u_HasMetallic", !packed); // suppress unpacked path
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
