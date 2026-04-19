#ifndef MANGORENDERING_PHYSICSWORLD_H
#define MANGORENDERING_PHYSICSWORLD_H

#include <memory>

#include <Jolt/Jolt.h>
#include <Jolt/Core/JobSystem.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/PhysicsSystem.h>

class PhysicsWorld {
public:
    static PhysicsWorld& Get();

    void Initialize();
    void Shutdown();
    void Step(float deltaTime);

    [[nodiscard]] bool IsInitialized() const { return m_initialized; }
    [[nodiscard]] JPH::BodyInterface& GetBodyInterface();
    [[nodiscard]] JPH::PhysicsSystem& GetSystem() { return m_physicsSystem; }

    PhysicsWorld(const PhysicsWorld&) = delete;
    PhysicsWorld& operator=(const PhysicsWorld&) = delete;

private:
    PhysicsWorld() = default;
    ~PhysicsWorld() = default;

    bool m_initialized = false;
    JPH::PhysicsSystem m_physicsSystem;
    std::unique_ptr<JPH::TempAllocator> m_tempAllocator;
    std::unique_ptr<JPH::JobSystem> m_jobSystem;
    std::unique_ptr<JPH::BroadPhaseLayerInterface> m_broadPhaseLayerInterface;
    std::unique_ptr<JPH::ObjectVsBroadPhaseLayerFilter> m_objectVsBroadPhaseLayerFilter;
    std::unique_ptr<JPH::ObjectLayerPairFilter> m_objectLayerPairFilter;
};

#endif //MANGORENDERING_PHYSICSWORLD_H
