
#include "MeshNode3d.h"

#include "Core/RenderApi.h"
#include "Renderer/Meshes/PrimitiveMesh.h"

// for inspector use
MeshNode3d::MeshNode3d(Shader* shader) : m_shader(shader), m_material(std::make_shared<Material>()) {
    Init();
}

MeshNode3d::MeshNode3d(std::shared_ptr<Mesh> mesh, Shader* shader) : m_shader(shader), m_mesh(std::move(mesh)), m_material(std::make_shared<Material>()) {
    Init();
}

Node3d * MeshNode3d::Clone() const {
    MeshNode3d* clone = new MeshNode3d(m_mesh, m_shader);

    clone->SetName(GetName());
    clone->SetVisible(IsVisible());
    clone->SetPosition(GetPosition());
    clone->SetRotation(GetRotation());
    clone->SetScale(GetScale());

    if (m_material) {
        clone->SetMaterial(m_material);
    }
    if (m_materialOverride) {
        clone->SetMaterialOverride(m_materialOverride);
    }

    for (const Node3d* child : GetChildren()) {
        clone->AddChild(child->Clone());
    }

    return clone;
}

void MeshNode3d::SubmitToRenderer(RenderApi& renderer) {
    if (!m_mesh || !IsVisible()) return;
    renderer.SubmitMesh(this);
}

void MeshNode3d::Init() {
    SetName("MeshNode3d");
    m_meshSlot = std::make_shared<PropertyHolder>();

    m_meshSlot->AddProperty("mesh_type",
        [this]() -> PropertyValue { return GetMeshTypeName(); },
        [this](const PropertyValue& v) { SetMeshByName(std::get<std::string>(v)); }
    );
    m_meshSlot->AddProperty("mesh",
        [this]() -> PropertyValue {
            if (!m_mesh) return std::shared_ptr<PropertyHolder>{};
            return std::static_pointer_cast<PropertyHolder>(m_mesh);
        },
        [](const PropertyValue&) {}
    );

    AddProperty("mesh",
        [this]() -> PropertyValue { return m_meshSlot; },
        [](const PropertyValue&) {}
    );
    AddProperty("material",
        [this]() -> PropertyValue { return std::static_pointer_cast<PropertyHolder>(m_material); },
        [](const PropertyValue&) {}
    );
}

std::string MeshNode3d::GetMeshTypeName() const {
    if (!m_mesh) return "None";
    if (dynamic_cast<CubeMesh*>(m_mesh.get())) return "Cube";
    if (dynamic_cast<SphereMesh*>(m_mesh.get())) return "Sphere";
    if (dynamic_cast<PlaneMesh*>(m_mesh.get())) return "Plane";
    if (dynamic_cast<QuadMesh*>(m_mesh.get())) return "Quad";
    if (dynamic_cast<CylinderMesh*>(m_mesh.get())) return "Cylinder";
    if (dynamic_cast<CapsuleMesh*>(m_mesh.get())) return "Capsule";
    return "Custom";
}

void MeshNode3d::SetMeshByName(const std::string& name) {
    if (name == "Cube") m_mesh = std::make_shared<CubeMesh>();
    else if (name == "Sphere") m_mesh = std::make_shared<SphereMesh>();
    else if (name == "Plane") m_mesh = std::make_shared<PlaneMesh>();
    else if (name == "Quad") m_mesh = std::make_shared<QuadMesh>();
    else if (name == "Cylinder") m_mesh = std::make_shared<CylinderMesh>();
    else if (name == "Capsule") m_mesh = std::make_shared<CapsuleMesh>();
    else if (name == "None") m_mesh = nullptr;
}
