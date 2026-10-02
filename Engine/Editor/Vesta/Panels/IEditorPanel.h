#pragma once

/**
 * @file IEditorPanel.h
 * @brief Editör panelleri için polimorfik temel arayüzü (interface) tanımlar.
 */

#include <string>

namespace vesta {

    /**
     * @class IEditorPanel
     * @brief Tüm editör panellerinin türemesi gereken saf sanal (pure virtual) sözleşme sınıfı.
     * @note Vesta sistemi (vesta_system) bu arayüz üzerinden panelleri polimorfik olarak yönetir.
     */
    class IEditorPanel {
    public:
        virtual ~IEditorPanel() = default;

        /**
         * @brief Panelin ImGui çizim (render) komutlarını çalıştırır.
         * @note Her karede (frame) vesta_system tarafından çağrılır.
         */
        virtual void render() = 0;

        /**
         * @brief Panelin arayüzde (sekmede) görünecek başlığını döndürür.
         */
        virtual const std::string& get_title() const = 0;

        /**
         * @brief Panelin anlık olarak açık/görünür olup olmadığını kontrol eder.
         */
        virtual bool is_open() const = 0;

        /**
         * @brief Panelin görünürlük durumunu ayarlar.
         *
         * @param open Panel görünür olacaksa true, gizlenecekse false.
         */
        virtual void set_open(bool open) = 0;
    };

} // namespace vesta