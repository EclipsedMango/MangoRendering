
#include "PortalNode3d.h"

REGISTER_NODE_TYPE(PortalNode3d)

void PortalNode3d::LinkTo(PortalNode3d *other) {
    if (other == this) {
        return;
    }

    if (m_linkedPortal == other) {
        return;
    }

    Unlink();

    m_linkedPortal = other;
    if (!other) {
        return;
    }

    if (other->m_linkedPortal && other->m_linkedPortal != this) {
        other->m_linkedPortal->m_linkedPortal = nullptr;
    }

    other->m_linkedPortal = this;
}

void PortalNode3d::Unlink() {
    if (!m_linkedPortal) {
        return;
    }

    PortalNode3d* other = m_linkedPortal;
    m_linkedPortal = nullptr;

    if (other->m_linkedPortal == this) {
        other->m_linkedPortal = nullptr;
    }
}

void PortalNode3d::LinkPair(PortalNode3d *a, PortalNode3d *b) {
    if (a) {
        a->LinkTo(b);
    } else if (b) {
        b->Unlink();
    }
}
