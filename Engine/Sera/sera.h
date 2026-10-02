#pragma once

/**
 * @file sera.h
 * @brief SERA (Scene & Entity Resource Architecture) sistemine erişim sağlayan ana arayüz sınıfı.
 */

#include "sera_session.h"
#include "../Scene/scene_session.h"
#include <memory>
#include <string>

struct EngineContext;

namespace sera {

    /**
     * @class sera
     * @brief Sahne kaydetme, yükleme ve doğrulama süreçlerini yürüten SERA alt sisteminin dış arayüzü.
     */
    class sera
    {
    public:
        sera() = default;
        ~sera() = default;

        /**
         * @brief SERA sistemini ilklendirir ve oturumu (session) başlatır.
         * @param ctx Motor bağlamı.
         */
        void init(EngineContext& ctx);

        /**
         * @brief Mevcut sahneyi doğrulamadan geçirir ve diske SERA formatında kaydeder.
         * @param filepath Kaydedilecek hedefin dosya yolu.
         * @param current_scene Kaydedilecek sahne verisi.
         * @return Kayıt işleminin başarı durumunu ve loglarını içeren doğrulama raporu.
         */
        validation_report save_scene(
            const std::string& filepath,
            const scene& current_scene);

        /**
         * @brief Diskteki SERA sahne dosyasını okur, doğrular ve belleğe yükler.
         * @param filepath Okunacak dosya yolu.
         * @param out_current_scene Verilerin yükleneceği hedef sahne nesnesi.
         * @return Yükleme işleminin başarı durumunu ve loglarını içeren doğrulama raporu.
         */
        validation_report load_scene(
            const std::string& filepath,
            scene& out_current_scene);

    private:
        std::unique_ptr<sera_session> m_sera_session; ///< SERA okuma/yazma oturumunun yöneticisi.
    };

} // namespace sera