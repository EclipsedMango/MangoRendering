
#include "Skybox.h"

static constexpr float cubeVertices[] = {
    -1, -1,  1,   1, -1,  1,   1,  1,  1,  -1,  1,  1,  // +Z
    -1, -1, -1,  -1,  1, -1,   1,  1, -1,   1, -1, -1,  // -Z
    -1,  1, -1,  -1,  1,  1,   1,  1,  1,   1,  1, -1,  // +Y
    -1, -1, -1,   1, -1, -1,   1, -1,  1,  -1, -1,  1,  // -Y
     1, -1, -1,   1,  1, -1,   1,  1,  1,   1, -1,  1,  // +X
    -1, -1, -1,  -1, -1,  1,  -1,  1,  1,  -1,  1, -1,  // -X
};

static constexpr uint32_t cubeIndices[] = {
    0,  1,  2,   2,  3,  0,
    4,  5,  6,   6,  7,  4,
    8,  9, 10,  10, 11,  8,
   12, 13, 14,  14, 15, 12,
   16, 17, 18,  18, 19, 16,
   20, 21, 22,  22, 23, 20,
};

Skybox::Skybox(std::vector<std::string>& faces) {
    m_texture = new Texture(faces);
    m_shader  = new Shader("../Assets/Shaders/skybox.vert", "../Assets/Shaders/skybox.frag");
    m_vao     = CreateCubeVao();
}

Skybox::~Skybox() {
    delete m_texture;
    delete m_shader;
    delete m_vao;
}

void Skybox::Draw(const glm::mat4& view, const glm::mat4& projection) const {
    const glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(view));

    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    m_shader->Bind();
    m_shader->SetMatrix4("u_View", viewNoTranslation);
    m_shader->SetMatrix4("u_Projection", projection);
    m_shader->SetInt("u_Skybox", 0);

    m_texture->Bind(0);
    m_vao->Bind();
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);

    glEnable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);  // restore
}

VertexArray* Skybox::CreateCubeVao() {
    // custom vertex struct, because vertex array struct too wasteful
    std::vector<Vertex> vertices;
    vertices.reserve(24);
    for (int i = 0; i < 24; i++) {
        Vertex v{};
        v.position = { cubeVertices[i*3], cubeVertices[i*3+1], cubeVertices[i*3+2] };
        vertices.push_back(v);
    }

    const std::vector indices(std::begin(cubeIndices), std::end(cubeIndices));
    return new VertexArray(vertices, indices);
}