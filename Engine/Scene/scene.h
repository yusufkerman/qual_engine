#pragma once

/**
 * @file scene.h
 * @brief Sahne varlıkları, ECS kayıt defteri, ışık paketleri ve çalışma zamanı betiklerini yöneten sınıf.
 */

#include <string>
#include <vector>
#include "../ECS/registry.h"
#include "../Systems/camera_system.h"
#include "../Renderer/light_batch.h"

struct EngineContext;

/**
 * @class scene
 * @brief Varlıkların (Entity), bileşenlerin (Component) ve çalışma zamanı sistemlerinin barındığı temel dünya nesnesi.
 */
class scene
{
public:
    scene() = default;
    ~scene() = default;

    /**
     * @brief Sahnede yeni bir boş varlık oluşturur ve listeye ekler.
     * @return Oluşturulan varlık tanıtıcısı (entity ID).
     */
    entity create_entity();

    /**
     * @brief Sahneyi sıfırlar, tüm varlıkları, kayıt defterini ve ışıkları temizler.
     */
    void new_scene();

    /**
     * @brief Sahnenin ECS kayıt defterine erişim sağlar.
     */
    registry& get_registry() noexcept;

    /**
     * @brief Sahnenin ECS kayıt defterine salt okunur erişim sağlar.
     */
    const registry& get_registry() const noexcept;

    /**
     * @brief Sahnedeki tüm varlıkların kimlik listesini döner.
     */
    [[nodiscard]] const std::vector<entity>& get_entities() const noexcept;

    /**
     * @brief Sahne varlık listesini günceller (Örn: Deserialization sonrası).
     */
    void set_entities(const std::vector<entity>& entities);

    /**
     * @brief Aktif render kamerasını ayarlar.
     */
    void set_active_camera(entity newCam) noexcept;

    /**
     * @brief Aktif render kamerası varlık kimliğini döner.
     */
    [[nodiscard]] entity get_active_camera() const noexcept;

    /**
     * @brief Sahnedeki aktif ışık bileşenlerini toplayarak light_batch paketini günceller.
     */
    void update_light_batch();

    /**
     * @brief PLAY moduna geçerken native script bileşenlerini ilklendirir ve on_create çağrılarını yürütür.
     */
    void on_runtime_start(EngineContext& ctx);

    /**
     * @brief Her kare çalışan native script on_update mantığını işletir.
     */
    void on_update_runtime(float deltaTime);

    /**
     * @brief PLAY modundan çıkarken script örneklerini yok eder ve belleklerini serbest bırakır.
     */
    void clean_up_scripts();

    /**
     * @brief Sahnenin ışık paketine erişim sağlar.
     */
    [[nodiscard]] light_batch& get_light_batch() noexcept
    {
        return lights;
    }

    /**
     * @brief Sahnenin ışık paketine salt okunur erişim sağlar.
     */
    [[nodiscard]] const light_batch& get_light_batch() const noexcept
    {
        return lights;
    }

private:
    registry reg;
    std::vector<entity> entities;

    entity activeCamera = 0;
    light_batch lights;
};