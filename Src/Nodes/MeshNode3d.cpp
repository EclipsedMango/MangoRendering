
#include "MeshNode3d.h"

#include "Core/RenderApi.h"
#include "Core/ResourceManager.h"
#include "Renderer/Meshes/PrimitiveMesh.h"

REGISTER_NODE_TYPE(MeshNode3d)

MeshNode3d::MeshNode3d() : m_material(std::make_shared<Material>()) {
    MeshNode3d::Init();
}

MeshNode3d::MeshNode3d(std::shared_ptr<Shader> shader) : m_shader(std::move(shader)), m_material(std::make_shared<Material>()) {
    MeshNode3d::Init();
}

MeshNode3d::MeshNode3d(std::shared_ptr<Mesh> mesh, std::shared_ptr<Shader> shader) : m_shader(std::move(shader)), m_mesh(std::move(mesh)), m_material(std::make_shared<Material>()) {
    MeshNode3d::Init();
}

std::unique_ptr<Node3d> MeshNode3d::Clone() {
    auto clone = std::make_unique<MeshNode3d>(m_mesh, m_shader);

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

    for (Node3d* child : GetChildren()) {
        clone->AddChild(child->Clone());
    }

    return clone;
}

void MeshNode3d::SetMeshByName(const std::string &name) {
    m_meshName = name;
    m_mesh = ResourceManager::Get().LoadMesh(name);
}

void MeshNode3d::Init() {
    SetName("MeshNode3d");
    m_meshSlot = std::make_shared<PropertyHolder>();

    AddProperty("shader_path",
        [this]() -> PropertyValue { return m_shaderPath; },
        [this](const PropertyValue& v) {
            m_shaderPath = std::get<std::string>(v);
            m_shader = ResourceManager::Get().GetShader(m_shaderPath);
            if (!m_shader) {
                std::cerr << "[MeshNode3d] WARNING: Shader '" << m_shaderPath << "' is null! Did you preload it?\n";
            }
        }
    );
    m_meshSlot->AddProperty("mesh_type",
        [this]() -> PropertyValue { return m_meshName; },
        [this](const PropertyValue& v) {
            m_meshName = std::get<std::string>(v);
            m_mesh = ResourceManager::Get().LoadMesh(m_meshName);
        }
    );

    m_meshSlot->AddProperty("mesh",[this]() -> PropertyValue {
            if (!m_mesh) return std::shared_ptr<PropertyHolder>{};
            return std::static_pointer_cast<PropertyHolder>(m_mesh);
        },[](const PropertyValue&) {}
    );

    AddProperty("mesh",
        [this]() -> PropertyValue { return m_meshSlot; },
        [this](const PropertyValue& v) {
            const auto& holder = std::get<std::shared_ptr<PropertyHolder>>(v);
            if (!holder) return;

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
            if (!holder) return;
            for (const auto& [key, prop] : holder->GetProperties()) {
                try {
                    m_material->Set(key, prop.getter());
                } catch (const std::exception& e) {
                    std::cerr << "[MeshNode3d] material property '" << key << "' error: " << e.what() << "\n";
                }
            }
        }
    );
}
