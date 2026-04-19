#ifndef MANGORENDERING_RIGIDBODY3D_H
#define MANGORENDERING_RIGIDBODY3D_H

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>

#include "Node3d.h"

class CollisionShape3d;

class RigidBody3d : public Node3d {
public:
    enum class BodyType {
        Static,
        Kinematic,
        Dynamic
    };

    RigidBody3d();
    ~RigidBody3d() override;

    [[nodiscard]] std::unique_ptr<Node3d> Clone() override;
    void PhysicsProcess(float deltaTime) override;

    void SetBodyType(BodyType type);
    void SetMass(float mass);
    void SetGravityScale(float gravityScale);
    void SetLinearVelocity(const glm::vec3& velocity);
    void SetAngularVelocity(const glm::vec3& velocity);
    void SetLockRotation(bool lockRotation);
    void SetSyncToPhysics(bool syncToPhysics);

    [[nodiscard]] BodyType GetBodyType() const { return m_bodyType; }
    [[nodiscard]] float GetMass() const { return m_mass; }
    [[nodiscard]] float GetGravityScale() const { return m_gravityScale; }
    [[nodiscard]] glm::vec3 GetLinearVelocity() const { return m_linearVelocity; }
    [[nodiscard]] glm::vec3 GetAngularVelocity() const { return m_angularVelocity; }
    [[nodiscard]] bool IsRotationLocked() const { return m_lockRotation; }
    [[nodiscard]] bool IsSyncToPhysicsEnabled() const { return m_syncToPhysics; }

    [[nodiscard]] std::string GetNodeType() const override { return "RigidBody3d"; }

private:
    static std::string BodyTypeToString(BodyType type);
    static BodyType BodyTypeFromString(const std::string& type);

    void EnsureBody();
    void RecreateBody();
    void DestroyBody();
    void PushTransformToPhysics(float deltaTime) const;
    void PullTransformFromPhysics();
    [[nodiscard]] CollisionShape3d* GetCollisionShapeChild() const;

    BodyType m_bodyType = BodyType::Dynamic;
    float m_mass = 1.0f;
    float m_gravityScale = 1.0f;
    glm::vec3 m_linearVelocity = glm::vec3(0.0f);
    glm::vec3 m_angularVelocity = glm::vec3(0.0f);
    bool m_lockRotation = false;
    bool m_syncToPhysics = true;

    bool m_pendingRecreate = true;
    bool m_missingShapeWarned = false;
    bool m_hasBody = false;
    CollisionShape3d* m_cachedShapeNode = nullptr;
    std::string m_cachedShapeType = "Box";
    glm::vec3 m_cachedShapeSize = glm::vec3(0.5f);
    JPH::BodyID m_bodyId {};
};

#endif //MANGORENDERING_RIGIDBODY3D_H
