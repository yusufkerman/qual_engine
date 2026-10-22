/**
 * @file physics_system.cpp
 * @brief Jolt Physics motoru yaşam döngüsü, bellek yönetimi ve sistem ilerletme uygulaması.
 */

#include "physics_system.h"
#include <Jolt/Core/IssueReporting.h>
#include <iostream>
#include "physics_api.h"

 // Jolt makro uyarılarını yerel olarak bastırmak için
JPH_SUPPRESS_WARNINGS

physics_system::physics_system() = default;

// API sınıfının tam tanımı bu dosyada olduğu için unique_ptr gereksinimi karşılanır.
physics_system::~physics_system() = default;

bool physics_system::init(scene& s)
{
    // Fizik komut arayüzünü (API) oluştur ve mevcut sahnenin kayıt defterine bağla
    m_api = std::make_unique<physics_api>(this, &s.get_registry());

    // Jolt bellek ayırıcı (allocator) altyapısını ilklendir
    JPH::RegisterDefaultAllocator();

    // Jolt nesne üretimi için genel fabrika (Factory) örneğini ayarla
    JPH::Factory::sInstance = new JPH::Factory();

    // Jolt temel çarpışma veri tiplerini sisteme tanıt
    JPH::RegisterTypes();

    // Simülasyon döngüsündeki geçici hesaplamalar için bellek ayırıcı (10 MB ayrılmıştır)
    mTempAllocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);

    // Fizik hesaplamalarını paralelleştirmek için çoklu iş parçacığı havuzu (Mevcut donanım sınırının 1 eksiği)
    mJobSystem = std::make_unique<JPH::JobSystemThreadPool>(
        JPH::cMaxPhysicsJobs,
        JPH::cMaxPhysicsBarriers,
        std::thread::hardware_concurrency() - 1
    );

    // Ana fizik sistemini ayrılan sınırlar ve kurulan filtreler eşliğinde başlat
    mPhysicsSystem = std::make_unique<JPH::PhysicsSystem>();
    mPhysicsSystem->Init(
        mMaxBodies,
        mNumBodyMutexes,
        mMaxBodyPairs,
        mMaxContactConstraints,
        mBPInterface,
        mObjectVsBPFilter,
        mObjectVsObjectFilter
    );

    return true;
}

void physics_system::update(float delta_time)
{
    // Jolt simülasyonunu belirtilen zaman adımı kadar ilerletir.
    // İdeal senaryolarda sabit adım (fixed time step) veya çarpışma adım parametreleri artırılabilir.
    constexpr int collision_steps = 1;
    mPhysicsSystem->Update(delta_time, collision_steps, mTempAllocator.get(), mJobSystem.get());
}

void physics_system::shutdown()
{
    if (mPhysicsSystem)
    {
        mPhysicsSystem.reset();
    }

    mJobSystem.reset();
    mTempAllocator.reset();

    JPH::UnregisterTypes();

    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;
}
physics_api& physics_system::api()
{
    if (!m_api)
    {
        static physics_api s_fallbackApi(nullptr, nullptr);
        return s_fallbackApi;
    }
    return *m_api;
}
