
#ifndef MANGORENDERING_MESHNODE3D_H
#define MANGORENDERING_MESHNODE3D_H

#include "RenderableNode3d.h"
#include "Renderer/Materials/Material.h"
#include "Renderer/Meshes/Mesh.h"
#include "Renderer/Shader.h"

class MeshNode3d : public RenderableNode3d {
public:
    explicit MeshNode3d(Shader* shader);
    MeshNode3d(std::shared_ptr<Mesh> mesh, Shader* shader);

    [[nodiscard]] Node3d* Clone() const override;

    void SubmitToRenderer(RenderApi& renderer) override;

    void SetMesh(std::shared_ptr<Mesh> mesh) { m_mesh = std::move(mesh); };
    void SetMaterial(const std::shared_ptr<Material> &material) { m_material = material; }
    void SetMaterialOverride(const std::shared_ptr<Material> &material) { m_materialOverride = material; }
    void ClearMaterialOverride() { m_materialOverride = nullptr; }

    [[nodiscard]] Mesh* GetMesh()   const { return m_mesh ? m_mesh.get() : nullptr; }
    [[nodiscard]] Shader* GetShader() const { return m_shader; }
    [[nodiscard]] Material& GetActiveMaterial() { return m_materialOverride ? *m_materialOverride : *m_material; }
    [[nodiscard]] const Material& GetActiveMaterial() const { return m_materialOverride ? *m_materialOverride : *m_material; }

    [[nodiscard]] std::shared_ptr<Mesh> GetMeshPtr() const { return m_mesh; }
    [[nodiscard]] std::shared_ptr<Material> GetMaterialPtr() { return m_material; }

private:
    void Init();
    [[nodiscard]] std::string GetMeshTypeName() const;
    void SetMeshByName(const std::string& name);

    std::shared_ptr<PropertyHolder> m_meshSlot;

    Shader* m_shader = nullptr;

    std::shared_ptr<Mesh> m_mesh;
    std::shared_ptr<Material> m_material;
    std::shared_ptr<Material> m_materialOverride;
};


#endif //MANGORENDERING_MESHNODE3D_H