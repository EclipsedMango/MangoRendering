
#include "MeshNode3d.h"

#include <iostream>

#include "Core/RenderApi.h"
#include "Core/ResourceManager.h"
#include "Renderer/Animation/Animator.h"

REGISTER_NODE_TYPE(MeshNode3d)

MeshNode3d::MeshNode3d() : m_material(std::make_shared<Material>()) {
    MeshNode3d::Init();
}

MeshNode3d::MeshNode3d(std::shared_ptr<Mesh> mesh) : m_mesh(std::move(mesh)), m_material(std::make_shared<Material>()) {
    MeshNode3d::Init();
}

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

    CopyBaseStateTo(*clone);

    for (Node3d* child : GetChildren()) {
        clone->AddChild(child->Clone());
    }

    return clone;
}

void MeshNode3d::Process(const float deltaTime) {
    Node3d::Process(deltaTime);
    if (m_animator) {
        m_animator->Update(deltaTime);
    }
}

void MeshNode3d::SetMeshByName(const std::string &name) {
    m_mesh = ResourceManager::Get().Load<Mesh>(name);
    if (m_meshSlot) {
        m_meshSlot->Set("mesh_type", name);
    }
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
            m_mesh = ResourceManager::Get().Load<Mesh>(name);
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
                m_mesh = nullptr;
            }
        }
    );

    AddProperty("mesh",
        [this]() -> PropertyValue { return m_meshSlot; },
        [this](const PropertyValue& v) {
            const auto& holder = std::get<std::shared_ptr<PropertyHolder>>(v);
            if (!holder) {
                m_mesh = nullptr;
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
    return m_animator && m_animator->HasSkeleton() && !m_animator->GetSkinMatrices().empty();
}

const std::vector<glm::mat4>& MeshNode3d::GetSkinMatrices() const {
    static constexpr std::vector<glm::mat4> kEmpty;
    if (!m_animator) {
        return kEmpty;
    }

    return m_animator->GetSkinMatrices();
}
