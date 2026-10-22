#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <memory>

// Jolt namespace'lerini temiz tutmak için kütüphane içindeki katman numaralarını öntanımlıyoruz
namespace layers
{
    static constexpr JPH::ObjectLayer NON_MOVING = 0;
    static constexpr JPH::ObjectLayer MOVING = 1;
    static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
}

namespace broad_phase_layers
{
    static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
    static constexpr JPH::BroadPhaseLayer MOVING(1);
    static constexpr JPH::uint NUM_LAYERS = 2;
}

// Jolt'un nesne katmanlarının birbiriyle çarpışıp çarpışamayacağını belirleyen sınıf
class object_layer_pair_filter_impl : public JPH::ObjectLayerPairFilter
{
public:
    virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override
    {
        switch (inObject1)
        {
        case layers::NON_MOVING:
            return inObject2 == layers::MOVING; // Statik nesneler sadece hareketli nesnelerle çarpışır
        case layers::MOVING:
            return true; // Hareketli nesneler her şeyle çarpışır
        default:
            return false;
        }
    }
};

// Geniş faz katman filtrelemesi (BroadPhase vs ObjectLayer)
class bplayer_interface_impl : public JPH::BroadPhaseLayerInterface
{
public:
    bplayer_interface_impl()
    {
        mObjectToBroadPhase[layers::NON_MOVING] = broad_phase_layers::NON_MOVING;
        mObjectToBroadPhase[layers::MOVING] = broad_phase_layers::MOVING;
    }

    virtual JPH::uint GetNumBroadPhaseLayers() const override
    {
        return broad_phase_layers::NUM_LAYERS;
    }

    virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
    {
        return mObjectToBroadPhase[inLayer];
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override
    {
        switch ((JPH::BroadPhaseLayer::Type)inLayer)
        {
        case (JPH::BroadPhaseLayer::Type)broad_phase_layers::NON_MOVING:	return "NON_MOVING";
        case (JPH::BroadPhaseLayer::Type)broad_phase_layers::MOVING:		return "MOVING";
        default:															return "INVALID";
        }
    }
#endif

private:
    JPH::BroadPhaseLayer mObjectToBroadPhase[layers::NUM_LAYERS];
};

class object_vs_broad_phase_layer_filter_impl : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
    virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override
    {
        switch (inLayer1)
        {
        case layers::NON_MOVING:
            return inLayer2 == broad_phase_layers::MOVING;
        case layers::MOVING:
            return true;
        default:
            return false;
        }
    }
};

// Ana Fizik Sistemi Yönetici Sınıfı
class physics_system
{
public:
    physics_system() = default;
    ~physics_system() = default;

    bool init();
    void update(float delta_time);
    void shutdown();

    JPH::PhysicsSystem* get_jolt_system() { return mPhysicsSystem.get(); }


private:
    std::unique_ptr<JPH::PhysicsSystem> mPhysicsSystem;
    std::unique_ptr<JPH::TempAllocatorImpl> mTempAllocator;
    std::unique_ptr<JPH::JobSystemThreadPool> mJobSystem;

    // Jolt filtreleme arayüz örnekleri (Ömürleri physics_system ile aynı olmalı)
    bplayer_interface_impl mBPInterface;
    object_vs_broad_phase_layer_filter_impl mObjectVsBPFilter;
    object_layer_pair_filter_impl mObjectVsObjectFilter;

    // Jolt için gerekli olan max limit tanımları
    static constexpr unsigned int mMaxBodies = 1024;
    static constexpr unsigned int mNumBodyMutexes = 0;
    static constexpr unsigned int mMaxBodyPairs = 1024;
    static constexpr unsigned int mMaxContactConstraints = 1024;
};