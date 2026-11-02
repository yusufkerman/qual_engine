#pragma once

/**
 * @file physics_api.h
 * @brief Oyun motoru ile Jolt Physics motoru arasındaki yerel (native) fizik komut arayüzü.
 */

#include <glm/glm.hpp>
#include "../ECS/entity.h"
#include "../ECS/registry.h"

class physics_system;

/**
 * @struct physics_raycast_result
 * @brief Raycast sorgusu sonucunda elde edilen çarpışma verisi.
 */
struct physics_raycast_result
{
    bool hit{ false };
    entity hit_entity{ 0 };
    glm::vec3 point{ 0.0f };
    glm::vec3 normal{ 0.0f };
    float distance{ 0.0f };
};

/**
 * @class physics_api
 * @brief Çalışma zamanında (runtime) fizik gövdelerine kuvvet uygulama, konum sorgulama ve yapısal değişiklik yapma işlevleri sağlar.
 */
class physics_api
{
public:
    /**
     * @brief API sarmalayıcısını başlatır.
     * @param phys Fizik alt sisteminin yöneticisi.
     * @param reg Sahne varlık kayıt defteri.
     */
    physics_api(physics_system* phys, registry* reg);

    /**
     * @brief İlgili varlığın fizik gövdesine doğrusal hız (linear velocity) atar.
     */
    void set_velocity(entity e, const glm::vec3& v);

    /**
     * @brief İlgili varlığın güncel doğrusal hız vektörünü okur.
     * @return Hız vektörü. Varlıkta fizik bileşeni yoksa (0,0,0) döner.
     */
    glm::vec3 get_velocity(entity e);

    /**
     * @brief Varlığın fizik gövdesine ani bir itme (impulse) uygular ve uyuyan gövdeyi uyandırır.
     */
    void add_impulse(entity e, const glm::vec3& impulse);

    /**
     * @brief Varlığın fizik gövdesine belirli bir dünya noktasında itme (impulse) uygulayarak doğrusal ve açısal momentum üretir.
     */
    void add_impulse_at_point(entity e, const glm::vec3& impulse, const glm::vec3& point);

    /**
     * @brief Fizik dünyasında bir ışın (raycast) fırlatarak ilk çarpan katı gövdeyi tespit eder.
     * @param origin Işının başlangıç konumu.
     * @param direction Işının doğrultusu.
     * @param max_distance Maksimum erişim mesafesi.
     * @param ignore_entity Çarpışma testinde yok sayılacak varlık (Örn. ateş eden oyuncu).
     * @return Çarpışma sonucu (physics_raycast_result).
     */
    physics_raycast_result raycast(const glm::vec3& origin, const glm::vec3& direction, float max_distance = 1000.0f, entity ignore_entity = 0);

    /**
     * @brief Kinematik bir gövdeyi belirtilen hedef konuma adım (delta time) süresince pürüzsüz taşır.
     */
    void move_kinematic(entity e, const glm::vec3& targetPos, float dt);

    /**
     * @brief Fizik motorundaki konum ve dönüşüm bilgisini okuyarak GLM model matrisine dönüştürür.
     * @return 4x4 Dönüşüm matrisi (Transform).
     */
    glm::mat4 get_jolt_transform(entity e);

    /**
     * @brief Varlığın Jolt fizik dünyasındaki gövdesini (Body) kalıcı olarak siler ve bellekten temizler.
     */
    void destroy_body(entity e);

    /**
     * @brief Varlığın çarpışma geometrisini (Collider Shape) sıfırlar ancak gövdeyi korur.
     */
    void remove_collider_shape(entity e);

private:
    physics_system* m_phys; ///< Fizik motoruna güvenli erişim işaretçisi.
    registry* m_reg;        ///< ECS kayıt defteri işaretçisi.
};
