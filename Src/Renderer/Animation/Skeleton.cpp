#include "Skeleton.h"

REGISTER_PROPERTY_TYPE(Skeleton)

Skeleton::Skeleton() {
    RegisterProperties();
}

void Skeleton::SetJoints(std::vector<Joint> joints) {
    m_joints = std::move(joints);
    RebuildNodeToJointIndex();
}

const Skeleton::Joint* Skeleton::GetJoint(const int index) const {
    if (index < 0 || index >= static_cast<int>(m_joints.size())) {
        return nullptr;
    }

    return &m_joints[static_cast<size_t>(index)];
}

int Skeleton::FindJointByNodeIndex(const int nodeIndex) const {
    if (const auto it = m_nodeToJointIndex.find(nodeIndex); it != m_nodeToJointIndex.end()) {
        return it->second;
    }

    return -1;
}

void Skeleton::RegisterProperties() {
    AddProperty("name",
        [this]() -> PropertyValue { return m_name; },
        [this](const PropertyValue& v) { m_name = std::get<std::string>(v); }
    );

    AddProperty("root_node_index",
        [this]() -> PropertyValue { return m_rootNodeIndex; },
        [this](const PropertyValue& v) { m_rootNodeIndex = std::get<int>(v); }
    );

    AddProperty("joint_count",
        [this]() -> PropertyValue { return static_cast<int>(m_joints.size()); },
        [](const PropertyValue&) {}
    );
}

void Skeleton::RebuildNodeToJointIndex() {
    m_nodeToJointIndex.clear();
    m_nodeToJointIndex.reserve(m_joints.size());

    for (size_t i = 0; i < m_joints.size(); ++i) {
        if (m_joints[i].nodeIndex >= 0) {
            m_nodeToJointIndex[m_joints[i].nodeIndex] = static_cast<int>(i);
        }
    }
}
