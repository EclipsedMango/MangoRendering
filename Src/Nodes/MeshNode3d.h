
#ifndef MANGORENDERING_MESHNODE3D_H
#define MANGORENDERING_MESHNODE3D_H

#include <cstdint>
#include <limits>

#include "RenderableNode3d.h"
#include "Renderer/Materials/Material.h"
#include "Renderer/Meshes/Mesh.h"

class Animator;
class UniformBuffer;
class Shader;

class MeshNode3d : public RenderableNode3d {
public:
    MeshNode3d();
    explicit MeshNode3d(std::shared_ptr<Mesh> mesh);
    ~MeshNode3d() override;

    [[nodiscard]] std::unique_ptr<Node3d> Clone() override;
    void Process(float deltaTime) override;

    void SetMesh(std::shared_ptr<Mesh> mesh);
    void SetMeshByName(const std::string& name);
    [[nodiscard]] Mesh* GetMesh() const { return m_mesh ? m_mesh.get() : nullptr; }
    [[nodiscard]] std::shared_ptr<Mesh> GetMeshPtr() const { return m_mesh; }

    void SetMaterial(const std::shared_ptr<Material> &material) { m_material = material; }
    void SetMaterialOverride(const std::shared_ptr<Material> &material) { m_materialOverride = material; }
    void ClearMaterialOverride() { m_materialOverride = nullptr; }

    [[nodiscard]] Material* GetActiveMaterial() { return m_materialOverride ? m_materialOverride.get() : m_material.get(); }
    [[nodiscard]] const Material* GetActiveMaterial() const { return m_materialOverride ? m_materialOverride.get() : m_material.get(); }
    [[nodiscard]] std::shared_ptr<Material> GetMaterialPtr() { return m_material; }
    void SetAnimator(std::shared_ptr<Animator> animator);
    [[nodiscard]] std::shared_ptr<Animator> GetAnimator() const { return m_animator; }
    void SetAnimatorAutoUpdate(bool enabled) { m_animatorAutoUpdate = enabled; }
    [[nodiscard]] bool IsAnimatorAutoUpdateEnabled() const { return m_animatorAutoUpdate; }
    [[nodiscard]] bool HasSkinning() const;
    void BindSkinning(const Shader& shader) const;
    [[nodiscard]] static float ConsumeFrameSkinUploadMs();

    [[nodiscard]] std::string GetNodeType() const override { return "MeshNode3d"; }

private:
    virtual void Init();
    void SyncSkinningBuffer() const;
    void RefreshSkinningFlags();

    std::shared_ptr<PropertyHolder> m_meshSlot;

    std::shared_ptr<Mesh> m_mesh;
    std::shared_ptr<Material> m_material;
    std::shared_ptr<Material> m_materialOverride;
    std::shared_ptr<Animator> m_animator;
    bool m_animatorAutoUpdate = true;

    bool m_meshHasSkinWeights = false;
    mutable std::unique_ptr<UniformBuffer> m_skinningUbo;
    mutable uint64_t m_uploadedPoseVersion = std::numeric_limits<uint64_t>::max();
    mutable int m_uploadedSkinCount = 0;

    static constexpr int MAX_SKIN_JOINTS = 128;
};


#endif //MANGORENDERING_MESHNODE3D_H
