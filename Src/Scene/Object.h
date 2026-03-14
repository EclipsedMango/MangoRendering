
#ifndef MANGORENDERING_OBJECT_H
#define MANGORENDERING_OBJECT_H

#include "../Renderer/Mesh.h"
#include "../Renderer/Shader.h"
#include "../Nodes/Transform.h"
#include "Renderer/Material.h"

class Object {
public:
    Transform transform;

    Object(Mesh* mesh, Shader* shader);
    ~Object() = default;

    [[nodiscard]] Mesh* GetMesh() const { return m_mesh; }
    [[nodiscard]] Shader* GetShader() const { return m_shader; }
    [[nodiscard]] Material& GetMaterial() { return m_material; }
    [[nodiscard]] const Material& GetMaterial() const { return m_material; }

private:
    Mesh* m_mesh = nullptr;
    Shader* m_shader = nullptr;
    Material m_material;
};


#endif //MANGORENDERING_OBJECT_H