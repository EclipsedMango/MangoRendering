
#include "ShadowRenderer.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <unordered_map>
#include <tracy/Tracy.hpp>

#include "Glad/glad.h"
#include "glm/gtc/matrix_transform.hpp"
#include "../Lights/GpuLights.h"
#include "Core/RenderApi.h"
#include "Core/ResourceManager.h"
#include "Renderer/Buffers/ShaderStorageBuffer.h"

namespace {
    constexpr uint32_t kInstanceDataBinding = 16;

    float ComputeMaxWorldScale(const glm::mat4& worldMatrix) {
        const float sx = glm::length(glm::vec3(worldMatrix[0]));
        const float sy = glm::length(glm::vec3(worldMatrix[1]));
        const float sz = glm::length(glm::vec3(worldMatrix[2]));
        return std::max({sx, sy, sz});
    }
}

ShadowRenderer::ShadowRenderer() {
    m_shadowDepthShader = ResourceManager::Get().LoadShader("ShadowDepth", "Engine://Shaders/shadow_depth.vert", "Engine://Shaders/shadow_depth.frag");
    m_pointShadowDepthShader = ResourceManager::Get().LoadShaderWithGeom("PointShadowDepth", "Engine://Shaders/point_shadow_depth.vert", "Engine://Shaders/point_shadow_depth.frag", "Engine://Shaders/point_shadow_depth.geom");
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

void ShadowRenderer::RenderDirectionalShadows(const CameraNode3d& camera, const glm::vec2& viewportSize) {
    ZoneScoped;
    if (m_directionalLights.empty()) {
        return;
    }

    if (m_shadowDrawItems.empty()) {
        return;
    }

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.5f, 4.0f);

    m_shadowDepthShader->Bind();

    for (size_t i = 0; i < m_directionalLights.size(); i++) {
        ZoneScopedN("DirectionalLightShadow");
        CascadedShadowMap* csm = m_cascadedShadowMaps[i];
        csm->SetDirection(m_directionalLights[i]->GetDirection());
        csm->Update(camera);

        for (int c = 0; c < CascadedShadowMap::NUM_CASCADES; c++) {
            ZoneScopedN("CascadeShadowPass");
            csm->BeginRender(c);
            m_shadowDepthShader->SetMatrix4("u_LightSpaceMatrix", csm->GetLightSpaceMatrix(c));

            for (const auto& [key, drawItems] : m_instancedBatches) {
                if (drawItems.empty()) continue;
                const ShadowDrawItem* representative = drawItems.front();
                const Material* mat = representative->material;
                RenderApi::ApplyMaterialCull(mat);

                m_shadowDepthShader->SetBool("u_UseSkinnedVertexBuffer", false);
                m_shadowDepthShader->SetBool("u_UseInstancing", true);
                m_shadowDepthShader->SetBool("u_AlphaScissor", mat->GetBlendMode() == BlendMode::AlphaScissor);
                m_shadowDepthShader->SetFloat("u_AlphaScissorThreshold", mat->GetAlphaScissorThreshold());
                m_shadowDepthShader->SetBool("u_HasDiffuse", mat->GetDiffuse() != nullptr);
                m_shadowDepthShader->SetBool("u_AlphaDitherShadow", mat->GetBlendMode() == BlendMode::AlphaBlend);

                if (mat->GetDiffuse()) {
                    mat->GetDiffuse()->Bind(0);
                    m_shadowDepthShader->SetInt("u_Diffuse", 0);
                }

                m_instanceDataScratch.clear();
                m_instanceDataScratch.reserve(drawItems.size());
                for (const ShadowDrawItem* item : drawItems) {
                    m_instanceDataScratch.push_back({
                        item->model,
                        item->normalMatrix
                    });
                }

                EnsureInstanceBuffer(m_instanceDataScratch.size());
                m_instanceSsbo->SetData(m_instanceDataScratch.data(), m_instanceDataScratch.size() * sizeof(InstanceDrawData), 0);
                m_instanceSsbo->Bind();

                representative->mesh->GetBuffer()->Bind();
                glDrawElementsInstanced(
                    GL_TRIANGLES,
                    representative->mesh->GetBuffer()->GetIndexCount(),
                    GL_UNSIGNED_INT,
                    nullptr,
                    static_cast<GLsizei>(m_instanceDataScratch.size())
                );
                m_shadowDrawCallCount++;
            }

            for (const ShadowDrawItem* item : m_singleDrawItems) {
                const Material* mat = item->material;
                RenderApi::ApplyMaterialCull(mat);

                m_shadowDepthShader->SetBool("u_UseInstancing", false);
                m_shadowDepthShader->SetBool("u_AlphaScissor", mat->GetBlendMode() == BlendMode::AlphaScissor);
                m_shadowDepthShader->SetFloat("u_AlphaScissorThreshold", mat->GetAlphaScissorThreshold());
                m_shadowDepthShader->SetBool("u_HasDiffuse", mat->GetDiffuse() != nullptr);
                m_shadowDepthShader->SetBool("u_AlphaDitherShadow", mat->GetBlendMode() == BlendMode::AlphaBlend);

                if (mat->GetDiffuse()) {
                    mat->GetDiffuse()->Bind(0);
                    m_shadowDepthShader->SetInt("u_Diffuse", 0);
                }

                m_shadowDepthShader->SetMatrix4("u_Model", item->model);
                item->node->BindSkinning(*m_shadowDepthShader);

                item->mesh->GetBuffer()->Bind();
                glDrawElements(GL_TRIANGLES, item->mesh->GetBuffer()->GetIndexCount(), GL_UNSIGNED_INT, nullptr);
                m_shadowDrawCallCount++;
            }

            CascadedShadowMap::EndRender();
        }
    }

    glDisable(GL_POLYGON_OFFSET_FILL);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glViewport(0, 0, static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y));
}

void ShadowRenderer::RenderPointLightShadows(const CameraNode3d& camera, const std::vector<PointLight*>& pointLights, const glm::vec2& viewportSize) {
    ZoneScoped;
    if (!m_pointShadowMap || !m_pointShadowDepthShader || pointLights.empty()) {
        return;
    }

    if (m_shadowDrawItems.empty()) {
        return;
    }

    EnsurePointShadowMetaBuffer(pointLights.size());
    EnsurePointShadowVpUniformLocation();

    m_pointShadowMetaScratch.resize(pointLights.size());
    for (auto& [slot, farPlane, bias, pad] : m_pointShadowMetaScratch) {
        slot = 0xFFFFFFFFu;
        farPlane = 1.0f;
        bias = 0.015f;
        pad = 0.0f;
    }

    m_shadowCandidatesScratch.clear();
    m_shadowCandidatesScratch.reserve(pointLights.size());
    for (uint32_t i = 0; i < static_cast<uint32_t>(pointLights.size()); i++) {
        m_shadowCandidatesScratch.push_back({ i, ScorePointLight(pointLights[i], camera) });
    }
    std::ranges::sort(m_shadowCandidatesScratch, [](const auto& a, const auto& b) { return a.score < b.score; });

    const uint32_t shadowCount = static_cast<uint32_t>(std::min(m_shadowCandidatesScratch.size(), static_cast<size_t>(MAX_SHADOWED_POINT_LIGHTS)));

    m_shadowedPointLightsDebug.clear();
    m_shadowedPointLightsDebug.reserve(shadowCount);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(3.0f, 12.0f);

    m_pointShadowDepthShader->Bind();

    for (uint32_t slot = 0; slot < shadowCount; slot++) {
        ZoneScopedN("PointLightShadow");
        const uint32_t lightIndex = m_shadowCandidatesScratch[slot].index;
        const PointLight* L = pointLights[lightIndex];

        const float distanceToCamera = glm::length(L->GetPosition() - camera.GetPosition());
        if (distanceToCamera > MAX_POINT_LIGHT_SHADOW_DISTANCE + L->GetRadius()) {
            continue;
        }

        const float farPlane = glm::max(L->GetRadius(), 0.5f);
        const float lightRadius = L->GetRadius();

        m_shadowedPointLightsDebug.push_back({
            lightIndex, slot,
            m_shadowCandidatesScratch[slot].score,
            lightRadius, farPlane,
            L->GetPosition(),
            glm::length(L->GetPosition() - camera.GetPosition())
        });

        m_pointShadowMetaScratch[lightIndex].slot = slot;
        m_pointShadowMetaScratch[lightIndex].farPlane = farPlane;
        m_pointShadowMetaScratch[lightIndex].bias = glm::clamp(farPlane * 0.005f, 0.015f, 0.1f);

        BuildPointShadowFaceMatrices(L->GetPosition(), 0.1f, farPlane, m_pointLightVpScratch.data());

        m_pointShadowDepthShader->SetVector3("u_LightPos", L->GetPosition());
        m_pointShadowDepthShader->SetFloat("u_FarPlane", farPlane);
        m_pointShadowDepthShader->SetInt("u_LightSlot", static_cast<int>(slot));

        if (m_pointVpArrayUniformLocation != -1) {
            glUniformMatrix4fv(m_pointVpArrayUniformLocation, 6, GL_FALSE, &m_pointLightVpScratch[0][0][0]);
        }

        m_pointShadowMap->BeginLight(slot);

        BuildPointLightBatches(L->GetPosition(), lightRadius);

        for (const auto& [key, drawItems] : m_filteredInstancedBatches) {
            if (drawItems.empty()) continue;
            const ShadowDrawItem* representative = drawItems.front();
            const Material* mat = representative->material;
            RenderApi::ApplyMaterialCull(mat);

            m_pointShadowDepthShader->SetBool("u_UseSkinnedVertexBuffer", false);
            m_pointShadowDepthShader->SetBool("u_UseInstancing", true);
            m_pointShadowDepthShader->SetBool("u_AlphaScissor", mat->GetBlendMode() == BlendMode::AlphaScissor);
            m_pointShadowDepthShader->SetFloat("u_AlphaScissorThreshold", mat->GetAlphaScissorThreshold());
            m_pointShadowDepthShader->SetBool("u_HasDiffuse", mat->GetDiffuse() != nullptr);
            m_pointShadowDepthShader->SetBool("u_AlphaDitherShadow", mat->GetBlendMode() == BlendMode::AlphaBlend);

            if (mat->GetDiffuse()) {
                mat->GetDiffuse()->Bind(0);
                m_pointShadowDepthShader->SetInt("u_Diffuse", 0);
            }

            m_instanceDataScratch.clear();
            m_instanceDataScratch.reserve(drawItems.size());
            for (const ShadowDrawItem* item : drawItems) {
                m_instanceDataScratch.push_back({
                    item->model,
                    item->normalMatrix
                });
            }

            EnsureInstanceBuffer(m_instanceDataScratch.size());
            m_instanceSsbo->SetData(m_instanceDataScratch.data(), m_instanceDataScratch.size() * sizeof(InstanceDrawData), 0);
            m_instanceSsbo->Bind();

            representative->mesh->GetBuffer()->Bind();
            glDrawElementsInstanced(
                GL_TRIANGLES,
                representative->mesh->GetBuffer()->GetIndexCount(),
                GL_UNSIGNED_INT,
                nullptr,
                static_cast<GLsizei>(m_instanceDataScratch.size())
            );
            m_shadowDrawCallCount += 6;
        }

        for (const ShadowDrawItem* item : m_filteredSingleDrawItems) {
            const Material* mat = item->material;
            RenderApi::ApplyMaterialCull(mat);

            m_pointShadowDepthShader->SetBool("u_UseInstancing", false);
            m_pointShadowDepthShader->SetBool("u_AlphaScissor", mat->GetBlendMode() == BlendMode::AlphaScissor);
            m_pointShadowDepthShader->SetFloat("u_AlphaScissorThreshold", mat->GetAlphaScissorThreshold());
            m_pointShadowDepthShader->SetBool("u_HasDiffuse", mat->GetDiffuse() != nullptr);
            m_pointShadowDepthShader->SetBool("u_AlphaDitherShadow", mat->GetBlendMode() == BlendMode::AlphaBlend);

            if (mat->GetDiffuse()) {
                mat->GetDiffuse()->Bind(0);
                m_pointShadowDepthShader->SetInt("u_Diffuse", 0);
            }

            m_pointShadowDepthShader->SetMatrix4("u_Model", item->model);
            item->node->BindSkinning(*m_pointShadowDepthShader);
            item->mesh->GetBuffer()->Bind();
            glDrawElements(GL_TRIANGLES, item->mesh->GetBuffer()->GetIndexCount(), GL_UNSIGNED_INT, 0);
            m_shadowDrawCallCount += 6;
        }
    }

    PointLightShadowMap::End();
    m_pointShadowMetaSsbo->SetData(m_pointShadowMetaScratch.data(), m_pointShadowMetaScratch.size() * sizeof(GPUPointShadowMeta), 0);
    glDisable(GL_POLYGON_OFFSET_FILL);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glViewport(0, 0, static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y));
}

void ShadowRenderer::BindShadowUniforms(const Shader& shader) const {
    for (size_t i = 0; i < m_cascadedShadowMaps.size(); i++) {
        const CascadedShadowMap* csm = m_cascadedShadowMaps[i];
        const auto& splits = csm->GetSplitDistances();
        const auto& nearSplits = csm->GetSplitNearDistances();

        for (int c = 0; c < CascadedShadowMap::NUM_CASCADES; c++) {
            const std::string idx = std::to_string(c);
            shader.SetMatrix4("u_LightSpaceMatrix[" + idx + "]", csm->GetLightSpaceMatrix(c));
            shader.SetFloat("u_CascadeSplits[" + idx + "]", splits[c]);
            shader.SetFloat("u_CascadeNearSplits[" + idx + "]", nearSplits[c]);
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

void ShadowRenderer::EnsureInstanceBuffer(const size_t instanceCount) {
    if (instanceCount == 0) {
        return;
    }

    const size_t bytes = instanceCount * sizeof(InstanceDrawData);
    if (!m_instanceSsbo || m_instanceSsbo->GetSize() < bytes) {
        m_instanceSsbo = std::make_unique<ShaderStorageBuffer>(bytes, kInstanceDataBinding);
    }
}

float ShadowRenderer::ScorePointLight(const PointLight* light, const CameraNode3d& camera) {
    const glm::vec3 delta = light->GetPosition() - camera.GetPosition();
    const float d2 = glm::dot(delta, delta);
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

void ShadowRenderer::EnsurePointShadowVpUniformLocation() {
    if (m_pointVpArrayUniformLocation != -2) {
        return;
    }

    m_pointVpArrayUniformLocation = glGetUniformLocation(m_pointShadowDepthShader->m_id, "u_LightVP[0]");
}

void ShadowRenderer::BuildShadowDrawItems(const std::vector<MeshNode3d*>& renderQueue) {
    ZoneScopedN("BuildShadowDrawItems");
    m_shadowDrawItems.clear();
    m_singleDrawItems.clear();
    m_instancedBatches.clear();

    m_shadowDrawItems.reserve(renderQueue.size());
    m_singleDrawItems.reserve(renderQueue.size());
    m_instancedBatches.reserve(renderQueue.size());

    for (const MeshNode3d* node : renderQueue) {
        const Material* mat = node->GetActiveMaterial();
        if (!mat || !mat->GetCastShadows()) {
            continue;
        }

        const Mesh* mesh = node->GetMesh();
        if (!mesh) {
            continue;
        }

        ShadowDrawItem item;
        item.node = node;
        item.mesh = mesh;
        item.material = mat;
        item.model = node->GetWorldMatrix();
        item.normalMatrix = node->GetNormalMatrix();
        item.worldCenter = glm::vec3(item.model * glm::vec4(mesh->GetBoundsCenter(), 1.0f));
        item.worldRadius = mesh->GetBoundsRadius() * ComputeMaxWorldScale(item.model);
        item.singleDraw = node->IsUnique() || node->HasSkinning();
        item.batchKey = BatchKey {
            mesh->GetResourceId(),
            mat->GetResourceId()
        };

        m_shadowDrawItems.push_back(item);
    }

    for (const ShadowDrawItem& item : m_shadowDrawItems) {
        if (item.singleDraw) {
            m_singleDrawItems.push_back(&item);
        } else {
            m_instancedBatches[item.batchKey].push_back(&item);
        }
    }
}

void ShadowRenderer::BuildPointLightBatches(const glm::vec3& lightPos, const float lightRadius) {
    m_filteredInstancedBatches.clear();
    m_filteredSingleDrawItems.clear();

    m_filteredInstancedBatches.reserve(m_instancedBatches.size());
    m_filteredSingleDrawItems.reserve(m_singleDrawItems.size());

    for (const ShadowDrawItem& item : m_shadowDrawItems) {
        const glm::vec3 delta = item.worldCenter - lightPos;
        const float influenceRadius = lightRadius + item.worldRadius;
        if (glm::dot(delta, delta) > influenceRadius * influenceRadius) {
            continue;
        }

        if (item.singleDraw) {
            m_filteredSingleDrawItems.push_back(&item);
        } else {
            m_filteredInstancedBatches[item.batchKey].push_back(&item);
        }
    }
}
