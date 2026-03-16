
#ifndef MANGORENDERING_MESHNODE3D_H
#define MANGORENDERING_MESHNODE3D_H

#include "RenderableNode3d.h"
#include "Renderer/Materials/Material.h"
#include "Renderer/Meshes/Mesh.h"
#include "Renderer/Shader.h"

class MeshNode3d : public RenderableNode3d {
public:
    MeshNode3d(Mesh* mesh, Shader* shader);

    void SubmitToRenderer(RenderApi& renderer) override;

    void SetMaterial(const std::shared_ptr<Material> &material) { m_material = material; }
    void SetMaterialOverride(const std::shared_ptr<Material> &material) { m_materialOverride = material; }
    void ClearMaterialOverride() { m_materialOverride = nullptr; }

    [[nodiscard]] Mesh* GetMesh() const { return m_mesh; }
    [[nodiscard]] Shader* GetShader() const { return m_shader; }
    [[nodiscard]] Material& GetActiveMaterial() { return m_materialOverride ? *m_materialOverride : *m_material; }
    [[nodiscard]] const Material& GetActiveMaterial() const { return m_materialOverride ? *m_materialOverride : *m_material; }

    std::shared_ptr<Material> GetMaterialPtr() { return m_material; }

private:
    Mesh* m_mesh = nullptr;
    Shader* m_shader = nullptr;

    std::shared_ptr<Material> m_material;
    std::shared_ptr<Material> m_materialOverride;
};


#endif //MANGORENDERING_MESHNODE3D_H