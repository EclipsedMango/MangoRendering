
#include "ShadowRenderer.h"

#include <algorithm>
#include <iostream>

#include "glad/gl.h"
#include "glm/gtc/matrix_transform.hpp"
#include "../Lights/GpuLights.h"
#include "Core/RenderApi.h"
#include "Core/ResourceManager.h"
#include "Renderer/Buffers/ShaderStorageBuffer.h"

namespace {
    constexpr int MAX_SKIN_JOINTS = 128;

    void UploadSkinningUniforms(const MeshNode3d* node, const Shader* shader) {
        if (!node->HasSkinning()) {
            shader->SetBool("u_Skinned", false);
            return;
        }

        shader->SetBool("u_Skinned", true);
        const auto& skinMatrices = node->GetSkinMatrices();
        const int jointCount = std::min(static_cast<int>(skinMatrices.size()), MAX_SKIN_JOINTS);
        shader->SetInt("u_SkinMatrixCount", jointCount);
        for (int i = 0; i < jointCount; ++i) {
            shader->SetMatrix4("u_SkinMatrices[" + std::to_string(i) + "]", skinMatrices[static_cast<size_t>(i)]);
        }
    }
}

ShadowRenderer::ShadowRenderer() {
    m_shadowDepthShader = ResourceManager::Get().LoadShader("ShadowDepth", "shadow_depth.vert", "shadow_depth.frag");
    m_pointShadowDepthShader = ResourceManager::Get().LoadShaderWithGeom("PointShadowDepth", "point_shadow_depth.vert", "point_shadow_depth.frag", "point_shadow_depth.geom");
    m_pointShadowMap = std::make_unique<PointLightShadowMap>(POINT_SHADOW_RES, MAX_SHADOWED_POINT_LIGHTS);
}

ShadowRenderer::~ShadowRenderer() {
    m_shadowDepthShader.reset();
    m_pointShadowDepthShader.reset();
    for (const CascadedShadowMap* csm : m_cascadedShadowMaps) {
        delete csm;
    }
}

void ShadowRenderer::AddDirectionalLight(DirectionalLight* light) {
    if (!light) {
        throw std::runtime_error("ShadowRenderer: null directional light");
    }

    if (m_directionalLights.size() >= MAX_DIR_LIGHTS) {
        throw std::runtime_error("ShadowRenderer: max directional lights reached");
    }

    m_directionalLights.push_back(light);
    m_cascadedShadowMaps.push_back(new CascadedShadowMap(light->GetDirection()));
}

void ShadowRenderer::RemoveDirectionalLight(DirectionalLight* light) {
    const auto it = std::ranges::find(m_directionalLights, light);
    if (it == m_directionalLights.end()) {
        return;
    }

    const size_t index = std::distance(m_directionalLights.begin(), it);
    delete m_cascadedShadowMaps[index];
    m_cascadedShadowMaps.erase(m_cascadedShadowMaps.begin() + index);
    m_directionalLights.erase(it);
}

void ShadowRenderer::RenderDirectionalShadows(const CameraNode3d& camera, const std::vector<MeshNode3d*>& renderQueue, const glm::vec2& viewportSize) {
    if (m_directionalLights.empty()) {
        return;
    }

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);

    m_shadowDepthShader->Bind();

    for (size_t i = 0; i < m_directionalLights.size(); i++) {
        CascadedShadowMap* csm = m_cascadedShadowMaps[i];
        csm->SetDirection(m_directionalLights[i]->GetDirection());
        csm->Update(camera);

        for (int c = 0; c < CascadedShadowMap::NUM_CASCADES; c++) {
            csm->BeginRender(c);
            m_shadowDepthShader->SetMatrix4("u_LightSpaceMatrix", csm->GetLightSpaceMatrix(c));

            for (const MeshNode3d* node : renderQueue) {
                const Material* mat = node->GetActiveMaterial();
                if (!mat->GetCastShadows()) continue;

                RenderApi::ApplyMaterialCull(mat);

                m_shadowDepthShader->SetBool("u_AlphaScissor", mat->GetBlendMode() == BlendMode::AlphaScissor);
                m_shadowDepthShader->SetFloat("u_AlphaScissorThreshold", mat->GetAlphaScissorThreshold());
                m_shadowDepthShader->SetBool("u_HasDiffuse", mat->GetDiffuse() != nullptr);
                m_shadowDepthShader->SetBool("u_AlphaDitherShadow", mat->GetBlendMode() == BlendMode::AlphaBlend);

                if (mat->GetDiffuse()) {
                    mat->GetDiffuse()->Bind(0);
                    m_shadowDepthShader->SetInt("u_Diffuse", 0);
                }

                m_shadowDepthShader->SetMatrix4("u_Model", node->GetWorldMatrix());
                UploadSkinningUniforms(node, m_shadowDepthShader.get());

                node->GetMesh()->GetBuffer()->Bind();
                glDrawElements(GL_TRIANGLES, node->GetMesh()->GetBuffer()->GetIndexCount(), GL_UNSIGNED_INT, nullptr);
                m_shadowDrawCallCount++;
            }

            CascadedShadowMap::EndRender();
        }
    }

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glViewport(0, 0, static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y));
}

void ShadowRenderer::RenderPointLightShadows(const CameraNode3d& camera, const std::vector<PointLight*>& pointLights, const std::vector<MeshNode3d*>& renderQueue, const glm::vec2& viewportSize) {
    if (!m_pointShadowMap || !m_pointShadowDepthShader || pointLights.empty()) {
        return;
    }

    EnsurePointShadowMetaBuffer(pointLights.size());

    std::vector<GPUPointShadowMeta> meta(pointLights.size());
    for (auto& [slot, farPlane, bias, pad] : meta) {
        slot = 0xFFFFFFFFu;
        farPlane = 1.0f;
        bias = 0.01f;
        pad = 0.0f;
    }

    std::vector<ShadowCandidate> candidates;
    candidates.reserve(pointLights.size());
    for (uint32_t i = 0; i < static_cast<uint32_t>(pointLights.size()); i++) {
        candidates.push_back({ i, ScorePointLight(pointLights[i], camera) });
    }
    std::ranges::sort(candidates, [](const auto& a, const auto& b) { return a.score < b.score; });

    const uint32_t shadowCount = static_cast<uint32_t>(std::min(candidates.size(), static_cast<size_t>(MAX_SHADOWED_POINT_LIGHTS)));

    m_shadowedPointLightsDebug.clear();
    m_shadowedPointLightsDebug.reserve(shadowCount);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);

    m_pointShadowDepthShader->Bind();

    for (uint32_t slot = 0; slot < shadowCount; slot++) {
        const uint32_t lightIndex = candidates[slot].index;
        const PointLight* L = pointLights[lightIndex];
        const float farPlane = glm::max(L->GetRadius(), 0.5f);
        const float lightRadius = L->GetRadius();

        m_shadowedPointLightsDebug.push_back({
            lightIndex, slot,
            candidates[slot].score,
            lightRadius, farPlane,
            L->GetPosition(),
            glm::length(L->GetPosition() - camera.GetPosition())
        });

        meta[lightIndex].slot = slot;
        meta[lightIndex].farPlane = farPlane;
        meta[lightIndex].bias = glm::clamp(farPlane * 0.0015f, 0.003f, 0.03f);

        glm::mat4 vp[6];
        BuildPointShadowFaceMatrices(L->GetPosition(), 0.1f, farPlane, vp);

        m_pointShadowDepthShader->SetVector3("u_LightPos", L->GetPosition());
        m_pointShadowDepthShader->SetFloat("u_FarPlane", farPlane);
        m_pointShadowDepthShader->SetInt("u_LightSlot", static_cast<int>(slot));

        for (int face = 0; face < 6; face++) {
            m_pointShadowDepthShader->SetMatrix4("u_LightVP[" + std::to_string(face) + "]", vp[face]);
        }

        m_pointShadowMap->BeginLight(slot);

        for (const MeshNode3d* node : renderQueue) {
            const Material* mat = node->GetActiveMaterial();
            if (!mat->GetCastShadows()) continue;

            const Mesh* mesh = node->GetMesh();
            const glm::vec3 worldCenter = glm::vec3(node->GetWorldMatrix() * glm::vec4(mesh->GetBoundsCenter(), 1.0f));
            const float worldRadius = mesh->GetBoundsRadius() * std::max({ node->GetScale().x, node->GetScale().y, node->GetScale().z });

            if (glm::length(worldCenter - L->GetPosition()) > lightRadius + worldRadius) {
                continue;
            }

            RenderApi::ApplyMaterialCull(node->GetActiveMaterial());

            m_pointShadowDepthShader->SetBool("u_AlphaScissor", mat->GetBlendMode() == BlendMode::AlphaScissor);
            m_pointShadowDepthShader->SetFloat("u_AlphaScissorThreshold", mat->GetAlphaScissorThreshold());
            m_pointShadowDepthShader->SetBool("u_HasDiffuse", mat->GetDiffuse() != nullptr);
            m_pointShadowDepthShader->SetBool("u_AlphaDitherShadow", mat->GetBlendMode() == BlendMode::AlphaBlend);

            if (mat->GetDiffuse()) {
                mat->GetDiffuse()->Bind(0);
                m_pointShadowDepthShader->SetInt("u_Diffuse", 0);
            }

            m_pointShadowDepthShader->SetMatrix4("u_Model", node->GetWorldMatrix());
            UploadSkinningUniforms(node, m_pointShadowDepthShader.get());
            node->GetMesh()->GetBuffer()->Bind();
            glDrawElements(GL_TRIANGLES, node->GetMesh()->GetBuffer()->GetIndexCount(), GL_UNSIGNED_INT, 0);
            m_shadowDrawCallCount += 6;
        }
    }

    PointLightShadowMap::End();
    m_pointShadowMetaSsbo->SetData(meta.data(), meta.size() * sizeof(GPUPointShadowMeta), 0);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glViewport(0, 0, static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y));
}

void ShadowRenderer::BindShadowUniforms(const Shader& shader) const {
    for (size_t i = 0; i < m_cascadedShadowMaps.size(); i++) {
        const CascadedShadowMap* csm = m_cascadedShadowMaps[i];
        const auto& splits = csm->GetSplitDistances();

        for (int c = 0; c < CascadedShadowMap::NUM_CASCADES; c++) {
            const std::string idx = std::to_string(c);
            shader.SetMatrix4("u_LightSpaceMatrix[" + idx + "]", csm->GetLightSpaceMatrix(c));
            shader.SetFloat("u_CascadeSplits[" + idx + "]", splits[c]);
            shader.SetFloat("u_CascadeWorldUnits[" + idx + "]", csm->GetWorldUnitsPerTexel(c));

            const int slot = CSM_TEXTURE_SLOT_BASE + c;
            glActiveTexture(GL_TEXTURE0 + slot);
            glBindTexture(GL_TEXTURE_2D, csm->GetCascadeTexture(c));
            shader.SetInt("u_ShadowMap" + idx, slot);
        }

        shader.SetInt("u_CascadeCount", CascadedShadowMap::NUM_CASCADES);
    }

    shader.SetInt("u_PointShadowMap", POINT_SHADOW_SLOT);
}

void ShadowRenderer::BindPointShadowTexture() const {
    glActiveTexture(GL_TEXTURE0 + POINT_SHADOW_SLOT);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, m_pointShadowMap->GetTexture());
}

void ShadowRenderer::EnsurePointShadowMetaBuffer(const size_t pointLightCount) {
    const size_t bytes = pointLightCount * sizeof(GPUPointShadowMeta);
    if (!m_pointShadowMetaSsbo || m_pointShadowMetaSsbo->GetSize() < bytes) {
        m_pointShadowMetaSsbo = std::make_unique<ShaderStorageBuffer>(bytes, 8);
    }
}

float ShadowRenderer::ScorePointLight(const PointLight* light, const CameraNode3d& camera) {
    const float d2 = glm::length(light->GetPosition() - camera.GetPosition());
    return d2 / glm::max(light->GetIntensity(), 0.001f);
}

void ShadowRenderer::BuildPointShadowFaceMatrices(const glm::vec3& lightPos, const float nearPlane, const float farPlane, glm::mat4 outVP[6]) {
    const glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, nearPlane, farPlane);

    const glm::vec3 dirs[6] = {
        { 1, 0, 0}, {-1, 0, 0}, { 0, 1, 0}, { 0,-1, 0}, { 0, 0, 1}, { 0, 0,-1}
    };
    const glm::vec3 ups[6] = {
        { 0,-1, 0}, { 0,-1, 0}, { 0, 0, 1}, { 0, 0,-1}, { 0,-1, 0}, { 0,-1, 0}
    };

    for (int i = 0; i < 6; i++) {
        outVP[i] = proj * glm::lookAt(lightPos, lightPos + dirs[i], ups[i]);
    }
}
