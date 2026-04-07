
#include "PortalNode3d.h"

REGISTER_NODE_TYPE(PortalNode3d)

PortalNode3d::PortalNode3d() {
    SetName("PortalNode");
    PortalNode3d::Init();
}

std::unique_ptr<Node3d> PortalNode3d::Clone() {
    auto clone = std::make_unique<PortalNode3d>();

    clone->SetName(GetName());
    if (auto* mat = GetMaterialPtr().get()) {
        clone->SetMaterial(std::make_shared<Material>(*mat));
    }

    clone->SetMesh(GetMeshPtr());
    clone->GetActiveMaterial()->SetShader(GetActiveMaterial()->GetShader());

    CopyBaseStateTo(*clone);

    for (Node3d* child : GetChildren()) {
        clone->AddChild(child->Clone());
    }

    return clone;
}

void PortalNode3d::LinkTo(PortalNode3d *other) {
    if (other == this) {
        return;
    }

    if (m_linkedPortal == other) {
        return;
    }

    Unlink();

    m_linkedPortal = other;
    m_linkedPortalName = other ? other->GetName() : "";
    if (!other) {
        return;
    }

    if (other->m_linkedPortal && other->m_linkedPortal != this) {
        other->m_linkedPortal->m_linkedPortal = nullptr;
    }

    other->m_linkedPortal = this;
    other->m_linkedPortalName = GetName();
}

void PortalNode3d::Unlink() {
    if (!m_linkedPortal) {
        return;
    }

    PortalNode3d* other = m_linkedPortal;
    m_linkedPortal = nullptr;
    m_linkedPortalName = "";

    if (other->m_linkedPortal == this) {
        other->m_linkedPortal = nullptr;
        other->m_linkedPortalName = "";
    }
}

void PortalNode3d::LinkPair(PortalNode3d *a, PortalNode3d *b) {
    if (a) {
        a->LinkTo(b);
    } else if (b) {
        b->Unlink();
    }
}

void PortalNode3d::Init() {
    AddProperty("is_linked",
        [this]() -> PropertyValue { return m_linkedPortal != nullptr; },
        [](const PropertyValue&) {}
    );

    AddProperty("linked_portal_name",
        [this]() -> PropertyValue { return m_linkedPortalName; },
        [this](const PropertyValue& v) { m_linkedPortalName = std::get<std::string>(v); }
    );
}
