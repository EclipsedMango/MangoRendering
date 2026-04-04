
#include "EquirectToCubemap.h"

#include <stdexcept>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Core/ResourceManager.h"
#include "Renderer/VertexArray.h"
#include "Renderer/Buffers/Framebuffer.h"
#include "Renderer/Meshes/CubeGeometry.h"
#include "Renderer/Meshes/Mesh.h"

std::shared_ptr<Texture> EquirectToCubemap::Convert(const std::string &hdrPath, const int faceSize) {
    GLint prevViewport[4];
    glGetIntegerv(GL_VIEWPORT, prevViewport);

    const glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.01f, 100.0f);

    static const glm::mat4 views[6] = {
        glm::lookAt(glm::vec3(0,0,0), glm::vec3( 1, 0, 0), glm::vec3(0,-1, 0)),
        glm::lookAt(glm::vec3(0,0,0), glm::vec3(-1, 0, 0), glm::vec3(0,-1, 0)),
        glm::lookAt(glm::vec3(0,0,0), glm::vec3( 0, 1, 0), glm::vec3(0, 0, 1)),
        glm::lookAt(glm::vec3(0,0,0), glm::vec3( 0,-1, 0), glm::vec3(0, 0,-1)),
        glm::lookAt(glm::vec3(0,0,0), glm::vec3( 0, 0, 1), glm::vec3(0,-1, 0)),
        glm::lookAt(glm::vec3(0,0,0), glm::vec3( 0, 0,-1), glm::vec3(0,-1, 0))
    };

    std::unique_ptr<Texture> equirect(Texture::LoadHDR(hdrPath));
    auto cubemap = std::make_shared<Texture>(faceSize, faceSize, GL_RGB16F, 1);

    const auto shader = ResourceManager::Get().LoadShader("EquirectToCube", "equirect_to_cubemap.vert", "equirect_to_cubemap.frag");
    auto cubeMesh = ResourceManager::Get().Load<Mesh>("Cube");

    const Framebuffer fbo(faceSize, faceSize, FramebufferType::ColorOnly);

    shader->Bind();
    shader->SetInt("u_EquirectMap", 0);
    shader->SetMatrix4("u_Projection", proj);
    equirect->Bind(0);

    glViewport(0, 0, faceSize, faceSize);

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    for (int i = 0; i < 6; i++) {
        fbo.Bind();

        shader->SetMatrix4("u_View", views[i]);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubemap->GetGLHandle(), 0);

        glClear(GL_COLOR_BUFFER_BIT);

        cubeMesh->GetBuffer()->Bind();
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
    }

    Framebuffer::Unbind();

    // restore state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);

    return cubemap;
}
