
#ifndef MANGORENDERING_OBJECT_H
#define MANGORENDERING_OBJECT_H

#include "Mesh.h"
#include "Shader.h"
#include "Texture.h"
#include "Transform.h"

class Object {
public:
    Transform transform;

    Object(Mesh* mesh, Shader* shader);
    void AddTexture(Texture* texture) { m_textures.push_back(texture); }

    [[nodiscard]] Mesh* GetMesh() const { return m_mesh; }
    [[nodiscard]] Shader* GetShader() const { return m_shader; }
    [[nodiscard]] const std::vector<Texture*>& GetTextures() const { return m_textures; }

private:
    Mesh* m_mesh = nullptr;
    Shader* m_shader = nullptr;
    std::vector<Texture*> m_textures;
};


#endif //MANGORENDERING_OBJECT_H