#pragma once

/**
 * @file inspector_view_model.h
 * @brief Inspector (Özellikler/Denetçi) paneli için Model-View-ViewModel (MVVM) yapısının görünüm modeli katmanını tanımlar.
 */

#include "../Adapters/IEditableWorld.h"
#include "../CoreState/editor_selection_manager.h"
#include "../Services/asset_ui_registry.h"
#include <vector>
#include <string>

namespace vesta
{
    /**
     * @enum inspector_mode
     * @brief Inspector panelinin anlık olarak hangi tür veriyi göstereceğini belirler.
     */
    enum class inspector_mode
    {
        none,   ///< Hiçbir şey seçili değil.
        entity, ///< Sahneden bir nesne (Entity) seçili, bileşenleri gösterilecek.
        asset   ///< Varlık tarayıcısından (Asset Browser) bir dosya seçili, özellikleri gösterilecek.
    };

    /**
     * @struct inspector_component_data
     * @brief UI tarafına aktarılacak olan bileşen (component) çizim verilerini tutar.
     */
    struct inspector_component_data
    {
        size_t hash = 0;                        ///< Bileşenin benzersiz tip kimliği.
        const ComponentUIInfo* ui = nullptr;    ///< Bileşeni çizecek olan UI bilgi yapısı.
        std::string display_name;               ///< Bileşenin ekranda görünecek adı.
        bool removable = true;                  ///< Kullanıcı bu bileşeni silebilir mi?
        bool default_open = true;               ///< UI açıldığında sekme varsayılan olarak açık mı olsun?
        bool expanded = false;                  ///< Sekmenin anlık açık/kapalı durumu.
    };

    /**
     * @struct asset_inspector_data
     * @brief Varlık (Asset) özellikleri çizimi için gereken verileri tutar.
     */
    struct asset_inspector_data
    {
        std::string asset_path;                 ///< Varlığın dosya yolu.
        std::string asset_type;                 ///< Varlığın türü (Örn: "material", "texture").
        const AssetUIInfo* ui = nullptr;        ///< Varlığı çizecek olan UI bilgi yapısı.
    };

    /**
     * @struct add_component_entry
     * @brief "Bileşen Ekle" (Add Component) menüsünde listelenecek elemanların verisi.
     */
    struct add_component_entry
    {
        size_t hash = 0;                        ///< Eklenecek bileşenin kimliği.
        const ComponentUIInfo* info = nullptr;  ///< Eklenecek bileşenin UI bilgisi.
    };

    /**
     * @class inspector_view_model
     * @brief Sahne nesnelerinin ve varlık dosyalarının özelliklerini UI katmanına sunan aracı sınıf.
     */
    class inspector_view_model
    {
    private:
        IEditableWorld& m_world;
        registry& m_registry;
        editor_selection_manager& m_selection;

        inspector_mode m_mode = inspector_mode::none;

    public:
        inspector_view_model(
            IEditableWorld& world,
            registry& reg,
            editor_selection_manager& selection);

        /**
         * @brief Seçim durumuna göre (Entity veya Asset) Inspector modunu günceller.
         */
        void update();
        void refresh();

        inspector_mode get_mode() const;

        /**
         * @brief Seçili nesneye eklenebilecek (henüz sahip olmadığı) bileşenlerin listesini döndürür.
         */
        std::vector<add_component_entry> get_addable_components() const;

        /**
         * @brief Seçili varlığın (Asset) arayüz çizimi için gereken verilerini döndürür.
         */
        asset_inspector_data get_asset_data() const;

        // ==========================================
        // UI KATMANI ERİŞİMİ
        // ==========================================

        registry& get_registry() const { return m_registry; }

        // ==========================================
        // SEÇİM DURUMU (SELECTION)
        // ==========================================

        bool has_selected_entity() const;
        entity get_selected_entity() const;
        const std::unordered_set<entity>& get_selected_entities() const;

        bool has_selected_asset() const;
        std::string get_selected_asset() const;

        // ==========================================
        // BİLEŞEN (COMPONENT) VERİLERİ
        // ==========================================

        std::vector<inspector_component_data> get_components() const;

        // ==========================================
        // EKLEME / SİLME (ADD / REMOVE)
        // ==========================================

        void request_add_component(size_t type_hash);
        void request_remove_component(size_t type_hash);
        void request_add_component_to_all(size_t type_hash);
        void request_remove_component_from_all(size_t type_hash);

        // ==========================================
        // DOĞRULAMA (VALIDATION)
        // ==========================================

        bool can_add(size_t type_hash) const;
        bool can_remove(size_t type_hash) const;
    };
} // namespace vesta