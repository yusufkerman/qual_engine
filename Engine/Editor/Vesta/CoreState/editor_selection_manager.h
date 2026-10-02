#pragma once

/**
 * @file editor_selection_manager.h
 * @brief Sahne varlıkları ve varlık dosyaları için çoklu seçim (multi-selection) ve bağlam yönetimini tanımlar.
 */

#include "../../../ECS/entity.h"
#include "selection_context.h"
#include <unordered_set>
#include <string>

namespace vesta {

    /**
     * @class editor_selection_manager
     * @brief Editör genelinde seçili olan varlıkları (Entity) ve dosyaları (Asset) eşzamanlı veya bağlamsal olarak tutan yönetici.
     */
    class editor_selection_manager {
    private:
        std::unordered_set<entity> m_selected_entities;
        std::unordered_set<std::string> m_selected_assets;
        selection_context_type m_active_context = selection_context_type::scene_entities;

    public:
        editor_selection_manager() = default;
        ~editor_selection_manager() = default;

        // ==========================================
        // GENEL KONTROLLER (GENERAL CONTROLS)
        // ==========================================

        /**
         * @brief Tüm varlık ve dosya seçimlerini temizler.
         */
        void clear_all();

        /**
         * @brief Aktif seçim bağlamını (Sahne veya Varlık) ayarlar.
         */
        void set_context(selection_context_type type);

        /**
         * @brief Güncel aktif seçim bağlamını döndürür.
         */
        selection_context_type get_context() const { return m_active_context; }

        /**
         * @brief Yalnızca sahne varlıklarının seçimini temizler.
         */
        void clear_entity_selection();

        /**
         * @brief Yalnızca dosya/varlık seçimini temizler.
         */
        void clear_asset_selection();

        /**
         * @brief Herhangi bir varlık veya dosya seçili mi kontrol eder.
         */
        bool has_any_selection() const;

        /**
         * @brief Seçim listelerinin tamamen boş olup olmadığını doğrular.
         */
        bool empty() const;

        // ==========================================
        // SAHNE VARLIKLARI SEÇİM API (ENTITY SELECTION)
        // ==========================================

        /**
         * @brief Bir varlığı seçili listesine ekler veya tekil seçim yapar.
         * @param e Seçilecek varlık kimliği.
         * @param accumulate True ise mevcut seçimi koruyup ekler (Ctrl basılı), false ise eski seçimi temizler.
         */
        void select_entity(entity e, bool accumulate = false);

        /**
         * @brief Bir varlığın seçimini kaldırır.
         */
        void deselect_entity(entity e);

        /**
         * @brief Varlığın anlık olarak seçili olup olmadığını kontrol eder.
         */
        bool is_entity_selected(entity e) const;

        /**
         * @brief Seçili varlıklar havuzundan ilk varlığı döndürür (Tekil denetimler için).
         * @return entity Varlık bulunursa kimliği, bulunamazsa 0.
         */
        entity get_first_selected_entity() const;

        const std::unordered_set<entity>& get_selected_entities() const { return m_selected_entities; }
        bool has_entity_selection() const { return !m_selected_entities.empty(); }

        // ==========================================
        // DOSYA / ASSET SEÇİM API (ASSET SELECTION)
        // ==========================================

        /**
         * @brief Bir varlık dosyasını seçili listesine ekler veya tekil seçim yapar.
         * @param asset_path_or_guid Dosya yolu veya GUID değeri.
         * @param accumulate True ise mevcut seçimi koruyup ekler, false ise temizler.
         */
        void select_asset(const std::string& asset_path_or_guid, bool accumulate = false);

        /**
         * @brief Bir dosyanın seçimini kaldırır.
         */
        void deselect_asset(const std::string& asset_path_or_guid);

        /**
         * @brief Dosyanın anlık olarak seçili olup olmadığını kontrol eder.
         */
        bool is_asset_selected(const std::string& asset_path_or_guid) const;

        /**
         * @brief Seçili dosyalardan ilkini döndürür.
         */
        std::string get_first_selected_asset() const;

        const std::unordered_set<std::string>& get_selected_assets() const { return m_selected_assets; }
        bool has_asset_selection() const { return !m_selected_assets.empty(); }
    };

} // namespace vesta