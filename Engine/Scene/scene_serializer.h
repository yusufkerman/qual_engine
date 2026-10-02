#pragma once

/**
 * @file scene_serializer.h
 * @brief Sahne durumunun JSON formatında serileştirilmesi ve dosyadan geri yüklenmesi işlemlerini yönetir.
 */

#include <string>

class scene;
struct EngineContext;

/**
 * @class scene_serializer
 * @brief Sahne varlıklarını, bileşenlerini ve varlık referanslarını JSON formatında saklayan ve okuyan statik servis sınıfı.
 */
class scene_serializer
{
public:
    scene_serializer() = delete;

    /**
     * @brief Sahnedeki varlıkları, bileşen verilerini ve doku referanslarını JSON dosyasına serileştirir.
     * @param filepath Hedef JSON dosya yolu.
     * @param currentScene Serileştirilecek kaynak sahne.
     * @param ctx Motor bağlamı.
     * @return İşlem başarılıysa true, dosya açılamazsa false döner.
     */
    static bool serialize(const std::string& filepath, const scene& currentScene, EngineContext& ctx);

    /**
     * @brief JSON dosyasından sahne durumunu okur, mevcut sahneyi sıfırlar ve dünyayı yeniden inşa eder.
     * @param filepath Okunacak JSON dosya yolu.
     * @param currentScene İnşa edilecek hedef sahne.
     * @param ctx Motor bağlamı.
     * @return İşlem başarılıysa true, dosya bulunamazsa veya format geçersizse false döner.
     */
    static bool deserialize(const std::string& filepath, scene& currentScene, EngineContext& ctx);
};