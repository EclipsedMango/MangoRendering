
#include "IBLPrecomputer.h"

#include <iostream>
#include <tracy/Tracy.hpp>

#include "Core/ResourceManager.h"
#include "glad/gl.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "Renderer/Shader.h"
#include "Renderer/Meshes/Mesh.h"

std::shared_ptr<Shader> IBLPrecomputer::m_irradianceShader;
std::shared_ptr<Shader> IBLPrecomputer::m_prefilterShader;

// 6 view matrices for rendering into each cubemap face
static const glm::mat4 captureViews[6] = {
    glm::lookAt(glm::vec3(0), glm::vec3( 1, 0, 0), glm::vec3(0,-1, 0)),
    glm::lookAt(glm::vec3(0), glm::vec3(-1, 0, 0), glm::vec3(0,-1, 0)),
    glm::lookAt(glm::vec3(0), glm::vec3( 0, 1, 0), glm::vec3(0, 0, 1)),
    glm::lookAt(glm::vec3(0), glm::vec3( 0,-1, 0), glm::vec3(0, 0,-1)),
    glm::lookAt(glm::vec3(0), glm::vec3( 0, 0, 1), glm::vec3(0,-1, 0)),
    glm::lookAt(glm::vec3(0), glm::vec3( 0, 0,-1), glm::vec3(0,-1, 0)),
};

static const glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

// unit cube for capturing
static std::unique_ptr<Mesh> CreateCaptureCube() {
    const std::vector<Vertex> vertices = {
        {{-1,-1,-1},{0,0,0},{0,0}}, {{ 1,-1,-1},{0,0,0},{0,0}},
        {{ 1, 1,-1},{0,0,0},{0,0}}, {{-1, 1,-1},{0,0,0},{0,0}},
        {{-1,-1, 1},{0,0,0},{0,0}}, {{ 1,-1, 1},{0,0,0},{0,0}},
        {{ 1, 1, 1},{0,0,0},{0,0}}, {{-1, 1, 1},{0,0,0},{0,0}},
    };
    const std::vector<uint32_t> indices = {
        0,2,1, 0,3,2, // -Z
        4,5,6, 4,6,7, // +Z
        0,1,5, 0,5,4, // -Y
        2,3,7, 2,7,6, // +Y
        0,4,7, 0,7,3, // -X
        1,2,6, 1,6,5, // +X
    };
    return std::make_unique<Mesh>(vertices, indices);
}

// this creates its own fbo and rbo using raw gl calls, because it felt overkill adding it to fbo class.
IBLPrecomputer::Result IBLPrecomputer::Compute(const Texture &envCubemap) {
    ZoneScoped;
    // create a single capture FBO and RBO reused across all passes
    GLuint captureFbo, captureRbo;
    glGenFramebuffers(1, &captureFbo);
    glGenRenderbuffers(1, &captureRbo);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFbo);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRbo);
    // size will be updated per pass
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, IRRADIANCE_SIZE, IRRADIANCE_SIZE);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRbo);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    const auto cube = CreateCaptureCube();

    m_irradianceShader = ResourceManager::Get().LoadShader("IrradianceShader", "ibl_capture.vert", "ibl_irradiance.frag");
    m_prefilterShader = ResourceManager::Get().LoadShader("IblPrefilterShader", "ibl_capture.vert", "ibl_prefilter.frag");

    if (!m_irradianceShader || !m_prefilterShader) {
        std::cerr << "[IBLPrecomputer] Failed to load shaders, aborting." << std::endl;
        return {};
    }

    Result result;
    result.irradiance = ComputeIrradiance(envCubemap, captureFbo, captureRbo, *cube);
    result.prefiltered = ComputePrefiltered(envCubemap, captureFbo, captureRbo, *cube);

    glDeleteFramebuffers(1, &captureFbo);
    glDeleteRenderbuffers(1, &captureRbo);

    // restore viewport to window size
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return result;
}

void IBLPrecomputer::Shutdown() {
    m_irradianceShader.reset();
    m_prefilterShader.reset();
}

std::unique_ptr<Texture> IBLPrecomputer::ComputeIrradiance(const Texture &env, const GLuint captureFbo, const GLuint captureRbo, const Mesh &cube) {
    ZoneScoped;
    auto irradiance = std::make_unique<Texture>(IRRADIANCE_SIZE, IRRADIANCE_SIZE, GL_RGB16F, 1);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFbo);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, IRRADIANCE_SIZE, IRRADIANCE_SIZE);

    m_irradianceShader->Bind();
    m_irradianceShader->SetInt("u_EnvMap", 0);
    m_irradianceShader->SetMatrix4("u_Projection", captureProjection);
    env.Bind(0);

    glViewport(0, 0, IRRADIANCE_SIZE, IRRADIANCE_SIZE);

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    for (int face = 0; face < 6; face++) {
        m_irradianceShader->SetMatrix4("u_View", captureViews[face]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, irradiance->GetGLHandle(), 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        cube.GetBuffer()->Bind();
        glDrawElements(GL_TRIANGLES, cube.GetBuffer()->GetIndexCount(), GL_UNSIGNED_INT, nullptr);
    }

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return irradiance;
}

std::unique_ptr<Texture> IBLPrecomputer::ComputePrefiltered(const Texture &env, const GLuint captureFbo, const GLuint captureRbo, const Mesh &cube) {
    ZoneScoped;
    auto prefiltered = std::make_unique<Texture>(PREFILTER_SIZE, PREFILTER_SIZE, GL_RGB16F, PREFILTER_MIP_LEVELS);

    m_prefilterShader->Bind();
    m_prefilterShader->SetInt("u_EnvMap", 0);
    m_prefilterShader->SetMatrix4("u_Projection", captureProjection);
    env.Bind(0);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFbo);

    for (int mip = 0; mip < PREFILTER_MIP_LEVELS; mip++) {
        const int mipWidth  = PREFILTER_SIZE >> mip;
        const int mipHeight = PREFILTER_SIZE >> mip;
        const float roughness = static_cast<float>(mip) / static_cast<float>(PREFILTER_MIP_LEVELS - 1);

        glBindRenderbuffer(GL_RENDERBUFFER, captureRbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
        glViewport(0, 0, mipWidth, mipHeight);

        m_prefilterShader->SetFloat("u_Roughness", roughness);

        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);

        for (int face = 0; face < 6; face++) {
            m_prefilterShader->SetMatrix4("u_View", captureViews[face]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, prefiltered->GetGLHandle(), mip);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            cube.GetBuffer()->Bind();
            glDrawElements(GL_TRIANGLES, cube.GetBuffer()->GetIndexCount(), GL_UNSIGNED_INT, nullptr);
        }

        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return prefiltered;
}
