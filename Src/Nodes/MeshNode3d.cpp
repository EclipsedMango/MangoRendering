
#include "MeshNode3d.h"

#include <chrono>
#include <iostream>

#include "Core/RenderApi.h"
#include "Core/ResourceManager.h"
#include "Renderer/Animation/Animator.h"
#include "Renderer/Buffers/UniformBuffer.h"

namespace {
    float s_frameSkinUploadMs = 0.0f;
}

REGISTER_NODE_TYPE(MeshNode3d)

MeshNode3d::MeshNode3d() : m_material(std::make_shared<Material>()) {
    MeshNode3d::Init();
}

MeshNode3d::MeshNode3d(std::shared_ptr<Mesh> mesh) : m_mesh(std::move(mesh)), m_material(std::make_shared<Material>()) {
    RefreshSkinningFlags();
    MeshNode3d::Init();
}

MeshNode3d::~MeshNode3d() = default;

std::unique_ptr<Node3d> MeshNode3d::Clone() {
    auto clone = std::make_unique<MeshNode3d>(m_mesh);

    clone->SetName(GetName());
    clone->SetVisible(IsVisible());
    clone->SetLocalTransform(GetLocalMatrix());

    if (m_material) {
        const auto materialCopy = std::make_shared<Material>(*m_material);
        clone->SetMaterial(materialCopy);
    }

    if (m_materialOverride) {
        const auto overrideCopy = std::make_shared<Material>(*m_materialOverride);
        clone->SetMaterialOverride(overrideCopy);
    }
    clone->SetAnimator(m_animator);
    clone->SetAnimatorAutoUpdate(m_animatorAutoUpdate);

    CopyBaseStateTo(*clone);

    for (Node3d* child : GetChildren()) {
        clone->AddChild(child->Clone());
    }

    return clone;
}

void MeshNode3d::Process(const float deltaTime) {
    Node3d::Process(deltaTime);
    if (m_animator && m_animatorAutoUpdate) {
        m_animator->Update(deltaTime);
        SyncSkinningBuffer();
    }
}

void MeshNode3d::SetMesh(std::shared_ptr<Mesh> mesh) {
    m_mesh = std::move(mesh);
    RefreshSkinningFlags();
    m_uploadedPoseVersion = std::numeric_limits<uint64_t>::max();
    m_uploadedSkinCount = 0;
}

void MeshNode3d::SetMeshByName(const std::string &name) {
    SetMesh(ResourceManager::Get().Load<Mesh>(name));
    if (m_meshSlot) {
        m_meshSlot->Set("mesh_type", name);
    }
}

void MeshNode3d::SetAnimator(std::shared_ptr<Animator> animator) {
    m_animator = std::move(animator);
    m_uploadedPoseVersion = std::numeric_limits<uint64_t>::max();
    m_uploadedSkinCount = 0;
}

void MeshNode3d::Init() {
    SetName("MeshNode3d");
    m_meshSlot = std::make_shared<PropertyHolder>();

    m_meshSlot->AddProperty("mesh_type",
        [this]() -> PropertyValue {
            return m_mesh ? "LoadedMesh" : "None";
        },
        [this](const PropertyValue& v) {
            const std::string name = std::get<std::string>(v);
            SetMesh(ResourceManager::Get().Load<Mesh>(name));
        }
    );

    m_meshSlot->AddProperty("mesh",
        [this]() -> PropertyValue {
            if (!m_mesh) return std::shared_ptr<PropertyHolder>{};
            return std::static_pointer_cast<PropertyHolder>(m_mesh);
        },
        [this](const PropertyValue& v) {
            const auto& holder = std::get<std::shared_ptr<PropertyHolder>>(v);
            if (!holder) {
                SetMesh(nullptr);
            }
        }
    );

    AddProperty("mesh",
        [this]() -> PropertyValue { return m_meshSlot; },
        [this](const PropertyValue& v) {
            const auto& holder = std::get<std::shared_ptr<PropertyHolder>>(v);
            if (!holder) {
                SetMesh(nullptr);
                return;
            }

            for (const auto& [key, prop] : holder->GetProperties()) {
                try {
                    m_meshSlot->Set(key, prop.getter());
                } catch (const std::exception& e) {
                    std::cerr << "[MeshNode3d] mesh property error: " << e.what() << "\n";
                }
            }
        }
    );

    AddProperty("material",
        [this]() -> PropertyValue { return std::static_pointer_cast<PropertyHolder>(m_material); },
        [this](const PropertyValue& v) {
            const auto& holder = std::get<std::shared_ptr<PropertyHolder>>(v);
            if (!holder) {
                m_material = nullptr;
                return;
            }

            if (const auto mat = std::dynamic_pointer_cast<Material>(holder)) {
                m_material = mat;
            }
        }
    );
}

bool MeshNode3d::HasSkinning() const {
    return m_meshHasSkinWeights && m_animator && m_animator->HasSkeleton() && !m_animator->GetSkinMatrices().empty();
}

void MeshNode3d::BindSkinning(const Shader& shader) const {
    if (!HasSkinning()) {
        shader.SetBool("u_Skinned", false);
        return;
    }

    SyncSkinningBuffer();
    const auto& skinMatrices = m_animator->GetSkinMatrices();
    const int skinCount = std::min(static_cast<int>(skinMatrices.size()), MAX_SKIN_JOINTS);
    if (skinCount <= 0 || !m_skinningUbo) {
        shader.SetBool("u_Skinned", false);
        return;
    }

    shader.SetBool("u_Skinned", true);
    shader.SetInt("u_SkinMatrixCount", skinCount);
    m_skinningUbo->Bind();
}

float MeshNode3d::ConsumeFrameSkinUploadMs() {
    const float value = s_frameSkinUploadMs;
    s_frameSkinUploadMs = 0.0f;
    return value;
}

void MeshNode3d::SyncSkinningBuffer() const {
    if (!HasSkinning()) {
        return;
    }

    const auto& skinMatrices = m_animator->GetSkinMatrices();
    const int skinCount = std::min(static_cast<int>(skinMatrices.size()), MAX_SKIN_JOINTS);
    if (skinCount <= 0) {
        return;
    }

    if (!m_skinningUbo) {
        m_skinningUbo = std::make_unique<UniformBuffer>(sizeof(glm::mat4) * static_cast<size_t>(MAX_SKIN_JOINTS), 9);
    }

    const uint64_t poseVersion = m_animator->GetPoseVersion();
    if (m_uploadedPoseVersion == poseVersion && m_uploadedSkinCount == skinCount) {
        return;
    }

    const auto uploadStart = std::chrono::high_resolution_clock::now();
    m_skinningUbo->SetData(skinMatrices.data(), sizeof(glm::mat4) * static_cast<size_t>(skinCount), 0);
    const auto uploadEnd = std::chrono::high_resolution_clock::now();
    s_frameSkinUploadMs += std::chrono::duration<float, std::milli>(uploadEnd - uploadStart).count();

    m_uploadedPoseVersion = poseVersion;
    m_uploadedSkinCount = skinCount;
}

void MeshNode3d::RefreshSkinningFlags() {
    m_meshHasSkinWeights = m_mesh && m_mesh->HasSkinWeights();
}
