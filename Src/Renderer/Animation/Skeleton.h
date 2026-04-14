#ifndef MANGORENDERING_SKELETON_H
#define MANGORENDERING_SKELETON_H

#include <string>
#include <unordered_map>
#include <vector>

#include <glm/mat4x4.hpp>

#include "Core/PropertyHolder.h"

class Skeleton : public PropertyHolder {
public:
    struct Joint {
        std::string name;
        int nodeIndex = -1;
        int parentJointIndex = -1;
        glm::mat4 restLocalTransform = glm::mat4(1.0f);
        glm::mat4 inverseBindMatrix = glm::mat4(1.0f);
    };

    Skeleton();
    ~Skeleton() override = default;

    [[nodiscard]] std::string GetPropertyHolderType() const override { return "Skeleton"; }

    void SetName(const std::string& name) { m_name = name; }
    void SetRootNodeIndex(const int nodeIndex) { m_rootNodeIndex = nodeIndex; }
    void SetJoints(std::vector<Joint> joints);

    [[nodiscard]] const std::string& GetName() const { return m_name; }
    [[nodiscard]] int GetRootNodeIndex() const { return m_rootNodeIndex; }
    [[nodiscard]] int GetJointCount() const { return static_cast<int>(m_joints.size()); }
    [[nodiscard]] const std::vector<Joint>& GetJoints() const { return m_joints; }
    [[nodiscard]] const Joint* GetJoint(int index) const;
    [[nodiscard]] int FindJointByNodeIndex(int nodeIndex) const;

private:
    void RegisterProperties();
    void RebuildNodeToJointIndex();

    std::string m_name;
    int m_rootNodeIndex = -1;
    std::vector<Joint> m_joints;
    std::unordered_map<int, int> m_nodeToJointIndex;
};

#endif //MANGORENDERING_SKELETON_H
