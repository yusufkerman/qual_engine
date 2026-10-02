#pragma once

/**
 * @file physics_system.h
 * @brief Jolt Physics motoru entegrasyonu, bellek yönetimi ve çarpışma filtrelemesi.
 */

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <memory>
#include "../Scene/scene.h"

class physics_api;

/**
 * @namespace layers
 * @brief Jolt fizik nesne katmanlarının tanımları.
 */
namespace layers
{
    static constexpr JPH::ObjectLayer NON_MOVING = 0; ///< Hareketsiz (Statik) nesneler.
    static constexpr JPH::ObjectLayer MOVING = 1;     ///< Hareketli (Dinamik ve Kinematik) nesneler.
    static constexpr JPH::ObjectLayer NUM_LAYERS = 2; ///< Toplam fizik katmanı sayısı.
}

/**
 * @namespace broad_phase_layers
 * @brief Jolt kaba faz (BroadPhase) katmanlarının tanımları.
 */
namespace broad_phase_layers
{
    static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
    static constexpr JPH::BroadPhaseLayer MOVING(1);
    static constexpr JPH::uint NUM_LAYERS = 2;
}

/**
 * @class object_layer_pair_filter_impl
 * @brief Nesne katmanlarının (Object Layer) birbiriyle çarpışma kurallarını belirleyen filtre.
 */
class object_layer_pair_filter_impl : public JPH::ObjectLayerPairFilter
{
public:
    virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override
    {
        switch (inObject1)
        {
        case layers::NON_MOVING:
            return inObject2 == layers::MOVING; // Statik nesneler sadece hareketli nesnelerle çarpışır.
        case layers::MOVING:
            return true; // Hareketli nesneler her şeyle çarpışır.
        default:
            return false;
        }
    }
};

/**
 * @class bplayer_interface_impl
 * @brief Nesne katmanlarını kaba faz (BroadPhase) katmanlarına eşleyen arayüz uygulaması.
 */
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
        case (JPH::BroadPhaseLayer::Type)broad_phase_layers::NON_MOVING: return "NON_MOVING";
        case (JPH::BroadPhaseLayer::Type)broad_phase_layers::MOVING:     return "MOVING";
        default:                                                         return "INVALID";
        }
    }
#endif

private:
    JPH::BroadPhaseLayer mObjectToBroadPhase[layers::NUM_LAYERS];
};

/**
 * @class object_vs_broad_phase_layer_filter_impl
 * @brief Nesne katmanları ile kaba faz katmanları arasındaki çarpışma kurallarını belirleyen filtre.
 */
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

/**
 * @class physics_system
 * @brief Jolt fizik motorunun yaşam döngüsünü, bellek tahsislerini ve iş parçacığı havuzunu yöneten ana sistem sınıfı.
 */
class physics_system
{
public:
    physics_system();
    ~physics_system();

    /**
     * @brief Jolt fizik motorunu, bellek ayırıcılarını ve çalışma arayüzlerini ilklendirir.
     * @param s Fizik dünyasının kurulacağı kaynak sahne.
     * @return Başlatma başarılıysa true döner.
     */
    bool init(scene& s);

    /**
     * @brief Fizik simülasyonunu bir adım (delta) ilerletir.
     * @param delta_time Geçen zaman (saniye cinsinden).
     */
    void update(float delta_time);

    /**
     * @brief Jolt motorunu güvenli bir şekilde kapatır ve tahsis edilen bellekleri serbest bırakır.
     */
    void shutdown();

    /**
     * @brief Fizik komutlarına ve olaylarına erişim sağlayan API nesnesini döner.
     */
    physics_api& api() { return *m_api; }

    /**
     * @brief Düşük seviye erişim için doğrudan Jolt PhysicsSystem işaretçisini döner.
     */
    JPH::PhysicsSystem* get_jolt_system() { return mPhysicsSystem.get(); }

private:
    std::unique_ptr<physics_api> m_api; ///< Yerel (native) fizik komut arayüzü.

    std::unique_ptr<JPH::PhysicsSystem> mPhysicsSystem;
    std::unique_ptr<JPH::TempAllocatorImpl> mTempAllocator;
    std::unique_ptr<JPH::JobSystemThreadPool> mJobSystem;

    // Jolt filtreleme arayüz örnekleri
    bplayer_interface_impl mBPInterface;
    object_vs_broad_phase_layer_filter_impl mObjectVsBPFilter;
    object_layer_pair_filter_impl mObjectVsObjectFilter;

    // Motor kısıtlamaları ve limitleri
    static constexpr unsigned int mMaxBodies = 1024;
    static constexpr unsigned int mNumBodyMutexes = 0;
    static constexpr unsigned int mMaxBodyPairs = 1024;
    static constexpr unsigned int mMaxContactConstraints = 1024;
};