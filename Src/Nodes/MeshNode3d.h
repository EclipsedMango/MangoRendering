
#ifndef MANGORENDERING_MESHNODE3D_H
#define MANGORENDERING_MESHNODE3D_H

#include "RenderableNode3d.h"
#include "Renderer/Material.h"
#include "Renderer/Mesh.h"
#include "Renderer/Shader.h"

class MeshNode3d : public RenderableNode3d {
public:
    MeshNode3d(Mesh* mesh, Shader* shader);

    void SubmitToRenderer(RenderApi& renderer) override;

    [[nodiscard]] Mesh* GetMesh() const { return m_mesh; }
    [[nodiscard]] Shader* GetShader() const { return m_shader; }
    [[nodiscard]] Material& GetMaterial() { return m_material; }
    [[nodiscard]] const Material& GetMaterial() const { return m_material; }

private:
    Mesh* m_mesh = nullptr;
    Shader* m_shader = nullptr;
    Material m_material;
};


#endif //MANGORENDERING_MESHNODE3D_H