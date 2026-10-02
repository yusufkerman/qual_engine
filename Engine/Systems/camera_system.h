#pragma once

/**
 * @file camera_system.h
 * @brief Sahnedeki aktif kamera bileşenini belirleyen ve kamera önceliklerini denetleyen sistem.
 */

#include "../ECS/entity.h"

class scene;

/**
 * @class camera_system
 * @brief Sahne varlıkları arasındaki kamera bileşenlerini tarayarak birincil (primary) kamerayı seçer ve sahneye bağlar.
 */
class camera_system
{
public:
    camera_system() = default;
    ~camera_system() = default;

    /**
     * @brief Sahnedeki kamera bileşenlerini tarar, birincil kamerayı günceller ve sahneye atar.
     * @param scn İncelenecek ve aktif kamerası güncellenecek sahne nesnesi.
     */
    void update(scene& scn);

private:
    entity m_lastCamera = 0; ///< Önceki karede seçili olan kamera varlık kimliği.
};