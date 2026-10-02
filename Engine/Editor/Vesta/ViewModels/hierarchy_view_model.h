#pragma once

/**
 * @file hierarchy_view_model.h
 * @brief Sahne Hiyerarşisi (Scene Hierarchy) paneli için Model-View-ViewModel (MVVM) yapısının görünüm modeli katmanını tanımlar.
 */

#include "../../ECS/entity.h"
#include "../../ECS/components.h"
#include "../Adapters/IEditableWorld.h"
#include "../CoreState/editor_selection_manager.h"
#include "../../../ECS/registry.h"
#include <string>
#include <vector>

namespace vesta {

    /**
     * @struct hierarchy_node_data
     * @brief Arayüz (UI) panelinin hiçbir mantık yürütmeden doğrudan çizebilmesi için tasarlanmış veri taşıyıcı (DTO) yapı.
     */
    struct hierarchy_node_data {
        entity id{ 0 };                 ///< Entity'nin benzersiz kimliği.
        std::string name;               ///< Entity'nin arayüzde gösterilecek adı.
        bool is_selected{ false };      ///< Arayüzde seçili olarak işaretlenip işaretlenmediği.
        bool has_children{ false };     ///< Alt öğelerinin (çocuklarının) olup olmadığı.
        entity parent_id{ 0 };          ///< Bağlı olduğu ebeveynin (parent) kimliği.

        /// Ağaç yapısında hiyerarşiyi dolaşabilmek için doğrudan alt öğelerin (çocukların) listesi.
        std::vector<entity> children;
    };

    /**
     * @class hierarchy_view_model
     * @brief Sahne hiyerarşisini UI paneline sunan, entity oluşturma, silme, ebeveyn atama ve yeniden adlandırma eylemlerini yöneten ViewModel sınıfı.
     */
    class hierarchy_view_model {
    private:
        IEditableWorld& m_world;
        registry& m_registry;
        editor_selection_manager& m_selection_manager;

        entity m_renaming_entity{ 0 };
        std::string m_rename_buffer;
        entity m_clipboard_entity{ 0 };

        /**
         * @brief Bir entity'nin doğrudan çocuklarını (kardeş zinciri üzerinden) toplar.
         * @note İsminde 'recursive' geçmesine rağmen mimari gereği sadece birinci derece çocukları toplar.
         *       Alt düğümler UI tarafından istendikçe çözümlenir.
         */
        void gather_children_recursive(entity parent, std::vector<entity>& out_children) const;

    public:
        hierarchy_view_model(IEditableWorld& world, registry& reg, editor_selection_manager& selection);
        ~hierarchy_view_model() = default;

        // ==========================================
        // VERİ HAZIRLIĞI (SADECE OKUMA / READ ONLY)
        // ==========================================

        /**
         * @brief Sahnedeki kök (parent değeri 0 olan) entity'leri getirir.
         */
        std::vector<hierarchy_node_data> get_root_nodes() const;

        /**
         * @brief Belirli bir entity'nin UI tarafında gösterilecek verilerini derler.
         */
        hierarchy_node_data get_node_data(entity e) const;

        // ==========================================
        // KULLANICI EYLEMLERİ (WRITE / INTENT)
        // ==========================================

        void request_create_entity(const std::string& default_name = "New Entity");
        void request_destroy_entity(entity e);
        void request_set_parent(entity child, entity parent);
        void request_rename_entity(entity e, const std::string& new_name);

        void request_select(entity e, bool accumulate);

        void request_copy(entity e);
        void request_paste();
        void request_duplicate(entity e);

        // ==========================================
        // SEÇİM DURUMU YÖNETİMİ (SELECTION)
        // ==========================================

        bool has_selection() const;
        entity get_selected_entity() const;
        bool is_selected(entity e) const;

        // ==========================================
        // YENİDEN ADLANDIRMA (RENAME) YÖNETİMİ
        // ==========================================

        bool is_renaming(entity e) const;
        void start_rename(entity e);
        void cancel_rename();
        void commit_rename();

        entity get_renaming_entity() const;
        std::string& get_rename_buffer();
        const std::string& get_rename_buffer() const;

        // ==========================================
        // PANO (CLIPBOARD) DURUMU
        // ==========================================

        bool has_clipboard() const;

        // ==========================================
        // EYLEM DOĞRULAMA (VALIDATION)
        // ==========================================

        bool can_copy() const;
        bool can_duplicate() const;
        bool can_delete() const;
        bool can_paste() const;

        // ==========================================
        // YÜKSEK SEVİYELİ (SEÇİLİ NESNE) EYLEMLERİ
        // ==========================================

        void request_copy_selected();
        void request_duplicate_selected();
        void request_delete_selected();
    };

} // namespace vesta