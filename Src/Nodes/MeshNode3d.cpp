
#include "MeshNode3d.h"
#include <stdexcept>

#include "Core/RenderApi.h"

MeshNode3d::MeshNode3d(Mesh* mesh, Shader* shader) : m_mesh(mesh), m_shader(shader) {
    if (m_mesh == nullptr) {
        throw std::runtime_error("MeshNode3d created with null mesh");
    }

    if (m_shader == nullptr) {
        throw std::runtime_error("MeshNode3d created with null shader");
    }
}

void MeshNode3d::SubmitToRenderer(RenderApi &renderer) {
    renderer.SubmitMesh(this);
}
