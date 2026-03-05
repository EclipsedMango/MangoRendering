
#include "Object.h"

Object::Object(Mesh* mesh, Shader* shader) : m_mesh(mesh), m_shader(shader), m_material() {
    if (m_mesh == nullptr) {
        throw std::runtime_error("Object created with null mesh");
    }

    if (m_shader == nullptr) {
        throw std::runtime_error("Object created with null shader");
    }

    m_mesh->Upload();
}