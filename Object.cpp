
#include "Object.h"

Object::Object(Mesh* mesh, Shader* shader) : m_mesh(mesh), m_shader(shader) {
    if (m_mesh == nullptr) {
        throw std::runtime_error("Object created with null mesh");
    }

    m_mesh->Upload();
}