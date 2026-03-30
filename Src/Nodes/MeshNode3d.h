
#ifndef MANGORENDERING_MESHNODE3D_H
#define MANGORENDERING_MESHNODE3D_H

#include "RenderableNode3d.h"
#include "Renderer/Materials/Material.h"
#include "Renderer/Meshes/Mesh.h"
#include "Renderer/Shader.h"

class MeshNode3d : public RenderableNode3d {
public:
    MeshNode3d();
    explicit MeshNode3d(std::shared_ptr<Shader> shader);
    MeshNode3d(std::shared_ptr<Mesh> mesh, std::shared_ptr<Shader> shader);

    [[nodiscard]] std::unique_ptr<Node3d> Clone() override;

    void SetMesh(std::shared_ptr<Mesh> mesh) { m_mesh = std::move(mesh); }
    void SetMeshByName(const std::string& name);
    void SetShader(const std::string& path) { m_shaderPath = path; }
    void SetShader(std::shared_ptr<Shader> shader) { m_shader = std::move(shader); }
    void SetMaterial(const std::shared_ptr<Material> &material) { m_material = material; }
    void SetMaterialOverride(const std::shared_ptr<Material> &material) { m_materialOverride = material; }
    void ClearMaterialOverride() { m_materialOverride = nullptr; }

    [[nodiscard]] std::string GetNodeType() const override { return "MeshNode3d"; }
    [[nodiscard]] Mesh* GetMesh() const { return m_mesh ? m_mesh.get() : nullptr; }
    [[nodiscard]] std::shared_ptr<Shader> GetShader() const { return m_shader; }
    [[nodiscard]] const std::string& GetShaderPath() const { return m_shaderPath; }
    [[nodiscard]] Material& GetActiveMaterial() { return m_materialOverride ? *m_materialOverride : *m_material; }
    [[nodiscard]] const Material& GetActiveMaterial() const { return m_materialOverride ? *m_materialOverride : *m_material; }

    [[nodiscard]] std::shared_ptr<Mesh> GetMeshPtr() const { return m_mesh; }
    [[nodiscard]] std::shared_ptr<Material> GetMaterialPtr() { return m_material; }

private:
    virtual void Init();

    std::shared_ptr<PropertyHolder> m_meshSlot;

    std::string m_shaderPath = "default";
    std::string m_meshName = "None";
    std::string m_meshTypeName = "None";

    std::shared_ptr<Shader> m_shader;
    std::shared_ptr<Mesh> m_mesh;
    std::shared_ptr<Material> m_material;
    std::shared_ptr<Material> m_materialOverride;
};


#endif //MANGORENDERING_MESHNODE3D_H