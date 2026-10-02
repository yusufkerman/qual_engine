/**
 * @file camera_system.cpp
 * @brief Kamera bileşeni doğrulama, birincil kamera seçimi ve geçiş loglama uygulaması.
 */

#include "camera_system.h"
#include "../Scene/scene.h"
#include "../ECS/components.h"
#include <iostream>

void camera_system::update(scene& scn)
{
    auto& reg = scn.get_registry();

    entity bestCamera = 0;
    bool foundPrimary = false;

    // 1. Birincil kamerayı bul ve birden fazla primary varsa fazlalıkları sıfırla
    for (const entity e : scn.get_entities())
    {
        if (!reg.has_component<camera_component>(e))
        {
            continue;
        }

        auto* cam = reg.get_component<camera_component>(e);

        if (cam->primary)
        {
            if (!foundPrimary)
            {
                foundPrimary = true;
                bestCamera = e;
            }
            else
            {
                cam->primary = false;
            }
        }
    }

    // 2. Birincil kamera atanmamışsa bulunan ilk kamerayı birincil yap
    if (!foundPrimary)
    {
        for (const entity e : scn.get_entities())
        {
            if (!reg.has_component<camera_component>(e))
            {
                continue;
            }

            reg.get_component<camera_component>(e)->primary = true;
            bestCamera = e;
            break;
        }
    }

    // 3. Değişiklik tespiti ve aktif kameranın atanması
    const entity prev = m_lastCamera;

    if (bestCamera != 0)
    {
        scn.set_active_camera(bestCamera);
    }

    if (bestCamera != prev)
    {
        std::cout << "[CameraSystem] Aktif kamera degisti: "
            << prev << " -> " << bestCamera << '\n';
        m_lastCamera = bestCamera;
    }
}