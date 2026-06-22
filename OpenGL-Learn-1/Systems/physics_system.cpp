// Dosya Yolu: Systems/physics_system.cpp
#include "physics_system.h"
#include <Jolt/Core/IssueReporting.h>
#include <iostream>

// Jolt için gerekli temel global macro kayıtları
JPH_SUPPRESS_WARNINGS

bool physics_system::init()
{
    // Jolt bellek allocator yapısını kurar
    JPH::RegisterDefaultAllocator();

    // Factory nesnesini oluşturur (Jolt içi nesne üretimleri için)
    JPH::Factory::sInstance = new JPH::Factory();

    // Jolt'un temel veri tiplerini kayıt eder
    JPH::RegisterTypes();

    // Simülasyon esnasında geçici bellek tahsisleri için allocator (10MB)
    mTempAllocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);

    // Çoklu iş parçacığı (Multithreading) için iş havuzu. 
    // Jolt'un ağır yükleri arkada işlemesi için (Mevcut thread sayısından 1 eksik kuruyoruz)
    mJobSystem = std::make_unique<JPH::JobSystemThreadPool>(
        JPH::cMaxPhysicsJobs,
        JPH::cMaxPhysicsBarriers,
        std::thread::hardware_concurrency() - 1
    );

    // Jolt ana sistemini ayağa kaldırıyoruz
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
    // Çarpışmaları çözmesi ve fiziği ilerletmesi için Jolt'u tetikliyoruz.
    // Gelişmiş motorlarda sabit adımlı (Fixed Update) olması istenir, şimdilik doğrudan delta_time veriyoruz.
    // Jolt adımları genellikle 1/60 oranında ideal çalışır.
    int collision_steps = 1;
    mPhysicsSystem->Update(delta_time, collision_steps, mTempAllocator.get(), mJobSystem.get());
}

void physics_system::shutdown()
{
    if (mPhysicsSystem)
        mPhysicsSystem.reset();

    mJobSystem.reset();
    mTempAllocator.reset();

    JPH::UnregisterTypes(); 

    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;
}