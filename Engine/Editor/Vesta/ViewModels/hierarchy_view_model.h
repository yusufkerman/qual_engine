#pragma once
#include "../../ECS/entity.h"
#include "../../ECS/components.h"
#include "../Adapters/IEditableWorld.h"
#include "../CoreState/editor_selection_manager.h"
#include "../../../ECS/registry.h"

#include <string>
#include <vector>

namespace vesta {

    // L5 Panelinin hiçbir mantık yürütmeden doğrudan çizmesi için tasarlanmış düz veri yapısı
    struct hierarchy_node_data {
        entity id{ 0 };
        std::string name;
        bool is_selected{ false };
        bool has_children{ false };
        entity parent_id{ 0 };

        // L5'teki bağlı listeyi dolaşabilmek için çocuk entity'ler
        std::vector<entity> children;
    };

    class hierarchy_view_model {
    private:
        IEditableWorld& m_world;
        registry& m_registry;
        editor_selection_manager& m_selection_manager;

        entity m_renaming_entity{ 0 };
        std::string m_rename_buffer;
        entity m_clipboard_entity{ 0 };

        void gather_children_recursive(entity parent, std::vector<entity>& out_children) const;

    public:
        hierarchy_view_model(IEditableWorld& world,
            registry& reg,
            editor_selection_manager& selection);

        ~hierarchy_view_model() = default;

        // ==========================================
        // DATA PREPARATION (READ ONLY)
        // ==========================================

        std::vector<hierarchy_node_data> get_root_nodes() const;
        hierarchy_node_data get_node_data(entity e) const;

        // ==========================================
        // INTENT ACTIONS (WRITE)
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
        // SELECTION
        // ==========================================

        bool has_selection() const;
        entity get_selected_entity() const;
        bool is_selected(entity e) const;

        // ==========================================
        // RENAME STATE
        // ==========================================

        bool is_renaming(entity e) const;
        void start_rename(entity e);
        void cancel_rename();

        entity get_renaming_entity() const;
        std::string& get_rename_buffer();
        const std::string& get_rename_buffer() const;

        void commit_rename();

        // ==========================================
        // CLIPBOARD
        // ==========================================

        bool has_clipboard() const;

        // ==========================================
        // VALIDATION
        // ==========================================

        bool can_copy() const;
        bool can_duplicate() const;
        bool can_delete() const;
        bool can_paste() const;

        // ==========================================
        // HIGH LEVEL ACTIONS
        // ==========================================

        void request_copy_selected();
        void request_duplicate_selected();
        void request_delete_selected();
    };

} // namespace vesta