#include "RigidBody3d.h"

#include <algorithm>
#include <iostream>
#include <vector>

#include <Jolt/Physics/Body/AllowedDOFs.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/MotionType.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/EActivation.h>

#include "CollisionShape3d.h"
#include "Core/PhysicsWorld.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

REGISTER_NODE_TYPE(RigidBody3d)

namespace {
    constexpr JPH::ObjectLayer LAYER_NON_MOVING = 0;
    constexpr JPH::ObjectLayer LAYER_MOVING = 1;

    JPH::Vec3 ToJoltVec3(const glm::vec3& vec) {
        return { vec.x, vec.y, vec.z };
    }

    JPH::Quat ToJoltQuat(const glm::quat& quat) {
        return { quat.x, quat.y, quat.z, quat.w };
    }

    glm::vec3 ToGlmVec3(const JPH::Vec3& vec) {
        return { vec.GetX(), vec.GetY(), vec.GetZ() };
    }

    glm::quat ToGlmQuat(const JPH::Quat& quat) {
        return glm::quat(quat.GetW(), quat.GetX(), quat.GetY(), quat.GetZ());
    }

    JPH::EMotionType ToJoltMotionType(const RigidBody3d::BodyType type) {
        switch (type) {
            case RigidBody3d::BodyType::Static: return JPH::EMotionType::Static;
            case RigidBody3d::BodyType::Kinematic: return JPH::EMotionType::Kinematic;
            case RigidBody3d::BodyType::Dynamic: return JPH::EMotionType::Dynamic;
        }

        return JPH::EMotionType::Dynamic;
    }

    glm::mat4 ComposeWorldMatrix(Node3d* node) {
        glm::mat4 world(1.0f);

        std::vector<Node3d*> chain;
        for (Node3d* cursor = node; cursor; cursor = cursor->GetParent()) {
            chain.push_back(cursor);
        }

        std::reverse(chain.begin(), chain.end());
        for (Node3d* segment : chain) {
            world *= segment->GetLocalMatrix();
        }

        return world;
    }

    JPH::RefConst<JPH::Shape> BuildShapeFromNode(const CollisionShape3d& shapeNode) {
        const glm::vec3 shapeSize = glm::max(shapeNode.GetShapeSize(), glm::vec3(0.01f));

        switch (shapeNode.GetShapeType()) {
            case CollisionShape3d::ShapeType::Box:
                return new JPH::BoxShape(ToJoltVec3(shapeSize));
            case CollisionShape3d::ShapeType::Sphere: {
                const float radius = std::max(std::max(shapeSize.x, shapeSize.y), shapeSize.z);
                return new JPH::SphereShape(std::max(radius, 0.01f));
            }
            case CollisionShape3d::ShapeType::Capsule: {
                const float radius = std::max(shapeSize.x, 0.01f);
                const float halfHeight = std::max(shapeSize.y * 0.5f, 0.01f);
                return new JPH::CapsuleShape(halfHeight, radius);
            }
        }

        return new JPH::BoxShape(ToJoltVec3(shapeSize));
    }
}

RigidBody3d::RigidBody3d() {
    SetName("RigidBody3d");

    AddProperty("body_type",
        [this]() -> PropertyValue { return BodyTypeToString(m_bodyType); },
        [this](const PropertyValue& v) { SetBodyType(BodyTypeFromString(std::get<std::string>(v))); }
    );
    AddProperty("mass",
        [this]() -> PropertyValue { return m_mass; },
        [this](const PropertyValue& v) { SetMass(std::get<float>(v)); }
    );
    AddProperty("gravity_scale",
        [this]() -> PropertyValue { return m_gravityScale; },
        [this](const PropertyValue& v) { SetGravityScale(std::get<float>(v)); }
    );
    AddProperty("linear_velocity",
        [this]() -> PropertyValue { return m_linearVelocity; },
        [this](const PropertyValue& v) { SetLinearVelocity(std::get<glm::vec3>(v)); }
    );
    AddProperty("angular_velocity",
        [this]() -> PropertyValue { return m_angularVelocity; },
        [this](const PropertyValue& v) { SetAngularVelocity(std::get<glm::vec3>(v)); }
    );
    AddProperty("lock_rotation",
        [this]() -> PropertyValue { return m_lockRotation; },
        [this](const PropertyValue& v) { SetLockRotation(std::get<bool>(v)); }
    );
    AddProperty("sync_to_physics",
        [this]() -> PropertyValue { return m_syncToPhysics; },
        [this](const PropertyValue& v) { SetSyncToPhysics(std::get<bool>(v)); }
    );
}

RigidBody3d::~RigidBody3d() {
    DestroyBody();
}

std::unique_ptr<Node3d> RigidBody3d::Clone() {
    auto clone = std::make_unique<RigidBody3d>();

    CopyBaseStateTo(*clone);
    clone->SetBodyType(m_bodyType);
    clone->SetMass(m_mass);
    clone->SetGravityScale(m_gravityScale);
    clone->SetLinearVelocity(m_linearVelocity);
    clone->SetAngularVelocity(m_angularVelocity);
    clone->SetLockRotation(m_lockRotation);
    clone->SetSyncToPhysics(m_syncToPhysics);
    clone->m_pendingRecreate = true;

    for (Node3d* child : GetChildren()) {
        clone->AddChild(child->Clone());
    }

    return clone;
}

void RigidBody3d::PhysicsProcess(const float deltaTime) {
    Node3d::PhysicsProcess(deltaTime);

    EnsureBody();
    if (!m_hasBody) {
        return;
    }

    if (m_bodyType == BodyType::Dynamic) {
        if (m_syncToPhysics) {
            PullTransformFromPhysics();
        }
    } else {
        PushTransformToPhysics(deltaTime);
    }
}

void RigidBody3d::SetBodyType(const BodyType type) {
    if (m_bodyType == type) {
        return;
    }

    m_bodyType = type;
    m_pendingRecreate = true;
}

void RigidBody3d::SetMass(float mass) {
    m_mass = std::max(mass, 0.001f);
    m_pendingRecreate = true;
}

void RigidBody3d::SetGravityScale(const float gravityScale) {
    m_gravityScale = gravityScale;
    m_pendingRecreate = true;
}

void RigidBody3d::SetLinearVelocity(const glm::vec3& velocity) {
    m_linearVelocity = velocity;
    if (m_hasBody) {
        PhysicsWorld::Get().GetBodyInterface().SetLinearVelocity(m_bodyId, ToJoltVec3(velocity));
    }
}

void RigidBody3d::SetAngularVelocity(const glm::vec3& velocity) {
    m_angularVelocity = velocity;
    if (m_hasBody && !m_lockRotation) {
        PhysicsWorld::Get().GetBodyInterface().SetAngularVelocity(m_bodyId, ToJoltVec3(velocity));
    }
}

void RigidBody3d::SetLockRotation(const bool lockRotation) {
    m_lockRotation = lockRotation;
    m_pendingRecreate = true;
}

void RigidBody3d::SetSyncToPhysics(const bool syncToPhysics) {
    m_syncToPhysics = syncToPhysics;
}

std::string RigidBody3d::BodyTypeToString(const BodyType type) {
    switch (type) {
        case BodyType::Static: return "Static";
        case BodyType::Kinematic: return "Kinematic";
        case BodyType::Dynamic: return "Dynamic";
    }

    return "Dynamic";
}

RigidBody3d::BodyType RigidBody3d::BodyTypeFromString(const std::string& type) {
    if (type == "Static") return BodyType::Static;
    if (type == "Kinematic") return BodyType::Kinematic;
    return BodyType::Dynamic;
}

CollisionShape3d* RigidBody3d::GetCollisionShapeChild() const {
    for (Node3d* child : GetChildren()) {
        if (auto* shape = dynamic_cast<CollisionShape3d*>(child)) {
            return shape;
        }
    }

    return nullptr;
}

void RigidBody3d::EnsureBody() {
    PhysicsWorld& world = PhysicsWorld::Get();
    if (!world.IsInitialized()) {
        return;
    }

    CollisionShape3d* shapeNode = GetCollisionShapeChild();
    if (!shapeNode) {
        if (!m_missingShapeWarned) {
            std::cerr << "[RigidBody3d Warning] '" << GetName() << "' has no CollisionShape3d child. Body will not be created.\n";
            m_missingShapeWarned = true;
        }

        DestroyBody();
        m_cachedShapeNode = nullptr;
        return;
    }

    m_missingShapeWarned = false;

    const std::string shapeType = CollisionShape3d::ShapeTypeToString(shapeNode->GetShapeType());
    const glm::vec3 shapeSize = shapeNode->GetShapeSize();
    if (shapeNode != m_cachedShapeNode || shapeType != m_cachedShapeType || shapeSize != m_cachedShapeSize) {
        m_cachedShapeNode = shapeNode;
        m_cachedShapeType = shapeType;
        m_cachedShapeSize = shapeSize;
        m_pendingRecreate = true;
    }

    if (!m_hasBody || m_pendingRecreate) {
        RecreateBody();
    }
}

void RigidBody3d::RecreateBody() {
    PhysicsWorld& world = PhysicsWorld::Get();
    if (!world.IsInitialized()) {
        return;
    }

    CollisionShape3d* shapeNode = GetCollisionShapeChild();
    if (!shapeNode) {
        DestroyBody();
        return;
    }

    DestroyBody();

    const JPH::RefConst<JPH::Shape> shape = BuildShapeFromNode(*shapeNode);
    const glm::mat4 worldMatrix = ComposeWorldMatrix(this);
    const glm::vec3 worldPos = glm::vec3(worldMatrix[3]);
    const glm::quat worldRot = glm::normalize(glm::quat_cast(worldMatrix));

    JPH::BodyCreationSettings settings(
        shape.GetPtr(),
        JPH::RVec3(worldPos.x, worldPos.y, worldPos.z),
        ToJoltQuat(worldRot),
        ToJoltMotionType(m_bodyType),
        m_bodyType == BodyType::Static ? LAYER_NON_MOVING : LAYER_MOVING
    );

    settings.mAllowDynamicOrKinematic = true;
    settings.mGravityFactor = m_gravityScale;
    settings.mLinearVelocity = ToJoltVec3(m_linearVelocity);
    settings.mAngularVelocity = ToJoltVec3(m_angularVelocity);
    settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
    settings.mMassPropertiesOverride.mMass = std::max(m_mass, 0.001f);

    if (m_lockRotation) {
        settings.mAllowedDOFs = JPH::EAllowedDOFs::TranslationX | JPH::EAllowedDOFs::TranslationY | JPH::EAllowedDOFs::TranslationZ;
    }

    JPH::BodyInterface& bodyInterface = world.GetBodyInterface();
    if (JPH::Body* body = bodyInterface.CreateBody(settings)) {
        m_bodyId = body->GetID();
        bodyInterface.AddBody(m_bodyId, m_bodyType == BodyType::Static ? JPH::EActivation::DontActivate : JPH::EActivation::Activate);
        m_hasBody = true;
        m_pendingRecreate = false;
    }
}

void RigidBody3d::DestroyBody() {
    if (!m_hasBody) {
        return;
    }

    PhysicsWorld& world = PhysicsWorld::Get();
    if (world.IsInitialized()) {
        JPH::BodyInterface& bodyInterface = world.GetBodyInterface();
        bodyInterface.RemoveBody(m_bodyId);
        bodyInterface.DestroyBody(m_bodyId);
    }

    m_hasBody = false;
    m_bodyId = JPH::BodyID();
}

void RigidBody3d::PushTransformToPhysics(const float deltaTime) const {
    if (!m_hasBody) return;

    PhysicsWorld& world = PhysicsWorld::Get();
    JPH::BodyInterface& bodyInterface = world.GetBodyInterface();

    const glm::mat4 worldMatrix = ComposeWorldMatrix(const_cast<RigidBody3d*>(this));
    const glm::vec3 worldPos = glm::vec3(worldMatrix[3]);
    const glm::quat worldRot = glm::normalize(glm::quat_cast(worldMatrix));

    if (m_bodyType == BodyType::Kinematic) {
        bodyInterface.MoveKinematic(
            m_bodyId,
            JPH::RVec3(worldPos.x, worldPos.y, worldPos.z),
            ToJoltQuat(worldRot),
            deltaTime
        );
    } else {
        bodyInterface.SetPositionAndRotation(
            m_bodyId,
            JPH::RVec3(worldPos.x, worldPos.y, worldPos.z),
            ToJoltQuat(worldRot),
            JPH::EActivation::DontActivate
        );
        bodyInterface.SetLinearVelocity(m_bodyId, ToJoltVec3(m_linearVelocity));
        if (m_lockRotation) {
            bodyInterface.SetAngularVelocity(m_bodyId, JPH::Vec3::sZero());
        } else {
            bodyInterface.SetAngularVelocity(m_bodyId, ToJoltVec3(m_angularVelocity));
        }
    }
}

void RigidBody3d::PullTransformFromPhysics() {
    if (!m_hasBody) {
        return;
    }

    PhysicsWorld& world = PhysicsWorld::Get();
    JPH::BodyInterface& bodyInterface = world.GetBodyInterface();

    JPH::RVec3 worldPos;
    JPH::Quat worldRot;
    bodyInterface.GetPositionAndRotation(m_bodyId, worldPos, worldRot);

    m_linearVelocity = ToGlmVec3(bodyInterface.GetLinearVelocity(m_bodyId));
    m_angularVelocity = ToGlmVec3(bodyInterface.GetAngularVelocity(m_bodyId));
    if (m_lockRotation) {
        m_angularVelocity = glm::vec3(0.0f);
    }

    glm::mat4 worldMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(worldPos.GetX(), worldPos.GetY(), worldPos.GetZ()));
    worldMatrix *= glm::mat4_cast(ToGlmQuat(worldRot));
    worldMatrix = glm::scale(worldMatrix, GetScale());

    glm::mat4 parentWorld = glm::mat4(1.0f);
    if (Node3d* parent = GetParent()) {
        parentWorld = ComposeWorldMatrix(parent);
    }

    const glm::mat4 local = glm::inverse(parentWorld) * worldMatrix;
    SetLocalTransform(local);
}
