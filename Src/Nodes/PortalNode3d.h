
#ifndef MANGORENDERING_PORTALNODE3D_H
#define MANGORENDERING_PORTALNODE3D_H

#include "MeshNode3d.h"

class CameraNode3d;

class PortalNode3d : public MeshNode3d {
public:
    PortalNode3d() { SetName("PortalNode"); }
    ~PortalNode3d() override { Unlink(); }

    void LinkTo(PortalNode3d* other);
    void Unlink();

    static void LinkPair(PortalNode3d* a, PortalNode3d* b);

    [[nodiscard]] PortalNode3d* GetLinkedPortal() const { return m_linkedPortal; }
    [[nodiscard]] bool IsLinked() const { return m_linkedPortal != nullptr; }

    [[nodiscard]] std::string GetNodeType() const override { return "PortalNode3d"; }

private:
    PortalNode3d* m_linkedPortal = nullptr;
};


#endif //MANGORENDERING_PORTALNODE3D_H