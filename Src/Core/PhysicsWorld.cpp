#include "PhysicsWorld.h"

#include <cstdarg>
#include <cstdio>
#include <thread>

#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>

namespace {
    namespace Layers {
        constexpr JPH::ObjectLayer NON_MOVING = 0;
        constexpr JPH::ObjectLayer MOVING = 1;
        constexpr JPH::ObjectLayer NUM_LAYERS = 2;
    }

    namespace BroadPhaseLayers {
        constexpr JPH::BroadPhaseLayer NON_MOVING(0);
        constexpr JPH::BroadPhaseLayer MOVING(1);
        constexpr uint32_t NUM_LAYERS = 2;
    }

    class BroadPhaseLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface {
    public:
        [[nodiscard]] uint GetNumBroadPhaseLayers() const override {
            return BroadPhaseLayers::NUM_LAYERS;
        }

        [[nodiscard]] JPH::BroadPhaseLayer GetBroadPhaseLayer(const JPH::ObjectLayer inLayer) const override {
            switch (inLayer) {
                case Layers::NON_MOVING: return BroadPhaseLayers::NON_MOVING;
                case Layers::MOVING: return BroadPhaseLayers::MOVING;
                default: return BroadPhaseLayers::MOVING;
            }
        }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
        [[nodiscard]] const char* GetBroadPhaseLayerName(const JPH::BroadPhaseLayer inLayer) const override {
            switch (inLayer.GetValue()) {
                case 0: return "NON_MOVING";
                case 1: return "MOVING";
                default: return "UNKNOWN";
            }
        }
#endif
    };

    class ObjectLayerPairFilterImpl final : public JPH::ObjectLayerPairFilter {
    public:
        [[nodiscard]] bool ShouldCollide(const JPH::ObjectLayer inObject1, const JPH::ObjectLayer inObject2) const override {
            if (inObject1 == Layers::NON_MOVING && inObject2 == Layers::NON_MOVING) {
                return false;
            }

            return true;
        }
    };

    class ObjectVsBroadPhaseLayerFilterImpl final : public JPH::ObjectVsBroadPhaseLayerFilter {
    public:
        [[nodiscard]] bool ShouldCollide(const JPH::ObjectLayer inLayer1, const JPH::BroadPhaseLayer inLayer2) const override {
            if (inLayer1 == Layers::NON_MOVING) {
                return inLayer2 == BroadPhaseLayers::MOVING;
            }

            return true;
        }
    };

    void TraceImpl(const char* inFormat, ...) {
        va_list list;
        va_start(list, inFormat);
        std::vprintf(inFormat, list);
        va_end(list);
    }

#if defined(JPH_ENABLE_ASSERTS)
    bool AssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, uint inLine) {
        std::fprintf(stderr, "Jolt assert failed: %s (%s) in %s:%u\n", inExpression, inMessage ? inMessage : "", inFile, inLine);
        return true;
    }
#endif
}

PhysicsWorld& PhysicsWorld::Get() {
    static PhysicsWorld instance;
    return instance;
}

void PhysicsWorld::Initialize() {
    if (m_initialized) {
        return;
    }

    JPH::RegisterDefaultAllocator();
    JPH::Trace = TraceImpl;
#if defined(JPH_ENABLE_ASSERTS)
    JPH::AssertFailed = AssertFailedImpl;
#endif

    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();

    m_tempAllocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);

    uint32_t workerCount = std::thread::hardware_concurrency();
    if (workerCount <= 1) {
        workerCount = 1;
    } else {
        workerCount -= 1;
    }

    m_jobSystem = std::make_unique<JPH::JobSystemThreadPool>(
        JPH::cMaxPhysicsJobs,
        JPH::cMaxPhysicsBarriers,
        workerCount
    );

    m_broadPhaseLayerInterface = std::make_unique<BroadPhaseLayerInterfaceImpl>();
    m_objectVsBroadPhaseLayerFilter = std::make_unique<ObjectVsBroadPhaseLayerFilterImpl>();
    m_objectLayerPairFilter = std::make_unique<ObjectLayerPairFilterImpl>();

    constexpr uint cMaxBodies = 8192;
    constexpr uint cNumBodyMutexes = 0;
    constexpr uint cMaxBodyPairs = 8192;
    constexpr uint cMaxContactConstraints = 8192;

    m_physicsSystem.Init(
        cMaxBodies,
        cNumBodyMutexes,
        cMaxBodyPairs,
        cMaxContactConstraints,
        *m_broadPhaseLayerInterface,
        *m_objectVsBroadPhaseLayerFilter,
        *m_objectLayerPairFilter
    );

    m_initialized = true;
}

void PhysicsWorld::Shutdown() {
    if (!m_initialized) {
        return;
    }

    m_objectLayerPairFilter.reset();
    m_objectVsBroadPhaseLayerFilter.reset();
    m_broadPhaseLayerInterface.reset();
    m_jobSystem.reset();
    m_tempAllocator.reset();

    JPH::UnregisterTypes();
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;

    m_initialized = false;
}

void PhysicsWorld::Step(const float deltaTime) {
    if (!m_initialized || deltaTime <= 0.0f) {
        return;
    }

    constexpr int collisionSteps = 1;
    m_physicsSystem.Update(deltaTime, collisionSteps, m_tempAllocator.get(), m_jobSystem.get());
}

JPH::BodyInterface& PhysicsWorld::GetBodyInterface() {
    return m_physicsSystem.GetBodyInterface();
}
