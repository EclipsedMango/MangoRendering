
#include "ClusterSystem.h"

#include <iostream>

#include "glad/gl.h"
#include "glm/gtc/matrix_transform.hpp"

ClusterSystem::ClusterSystem() {
    m_clusterShader = std::make_unique<Shader>("../Assets/Shaders/cluster_build.comp");
    m_cullShader    = std::make_unique<Shader>("../Assets/Shaders/light_cull.comp");

    constexpr size_t aabbSize  = NUM_CLUSTERS * 2 * sizeof(glm::vec4);
    constexpr size_t indexSize = NUM_CLUSTERS * MAX_LIGHTS_PER_CLUSTER * sizeof(uint32_t);
    constexpr size_t gridSize  = NUM_CLUSTERS * sizeof(glm::uvec4);

    m_clusterAabbSsbo = std::make_unique<ShaderStorageBuffer>(aabbSize, 4);
    m_lightIndexSsbo  = std::make_unique<ShaderStorageBuffer>(indexSize, 5);
    m_lightGridSsbo   = std::make_unique<ShaderStorageBuffer>(gridSize, 6);
    m_globalCountSsbo = std::make_unique<ShaderStorageBuffer>(sizeof(uint32_t), 7);
}

void ClusterSystem::Rebuild(const Camera& camera, const glm::vec2& viewportSize) {
    if (!m_clusterShader) {
        std::cerr << "ClusterSystem::Rebuild: no cluster shader\n";
        return;
    }

    m_clusterShader->Bind();
    m_clusterShader->SetMatrix4("u_InverseProjection", glm::inverse(camera.GetProjectionMatrix()));
    m_clusterShader->SetVector2("u_ScreenDimensions",  viewportSize);
    m_clusterShader->SetFloat("u_ZNear",               camera.GetNearPlane());
    m_clusterShader->SetFloat("u_ZFar",                camera.GetFarPlane());

    m_clusterAabbSsbo->Bind();
    m_clusterShader->Dispatch(CLUSTER_DIM_X, CLUSTER_DIM_Y, CLUSTER_DIM_Z);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void ClusterSystem::Cull(const ShaderStorageBuffer* pointLightSsbo, const ShaderStorageBuffer* spotLightSsbo) {
    if (!m_cullShader) {
        std::cerr << "ClusterSystem::Cull: no cull shader\n";
        return;
    }

    m_cullShader->Bind();

    constexpr uint32_t zero = 0;
    m_globalCountSsbo->Bind();
    glClearNamedBufferData(m_globalCountSsbo->GetId(), GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);

    m_lightGridSsbo->Bind();
    glClearNamedBufferData(m_lightGridSsbo->GetId(), GL_RGBA32UI, GL_RGBA_INTEGER, GL_UNSIGNED_INT, nullptr);

    m_clusterAabbSsbo->Bind();
    m_lightIndexSsbo->Bind();
    if (pointLightSsbo) pointLightSsbo->Bind();
    if (spotLightSsbo)  spotLightSsbo->Bind();

    m_cullShader->Dispatch(CLUSTER_DIM_X, CLUSTER_DIM_Y, CLUSTER_DIM_Z);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}