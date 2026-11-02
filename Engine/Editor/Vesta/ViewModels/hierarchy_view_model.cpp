/**
 * @file hierarchy_view_model.cpp
 * @brief Sahne Hiyerarşisi (Hierarchy) görünüm modelinin mantıksal uygulaması.
 */

#include "hierarchy_view_model.h"
#include "../CoreState/command_stack.h"
#include "../CoreState/Commands/spawn_entity_command.h"
#include "../CoreState/Commands/destroy_entity_command.h"
#include "../CoreState/Commands/reparent_entity_command.h"
#include "../CoreState/Commands/rename_entity_command.h"
#include "../CoreState/Commands/compound_command.h"
#include <algorithm>
#include <unordered_set>

namespace vesta {

    namespace {

        bool is_ancestor_in_set(const registry& reg, entity e, const std::unordered_set<entity>& set) {
            const auto* hc = reg.get_component<hierarchy_component>(e);
            entity cur = hc ? hc->parent : 0;
            int safety = 0;
            while (cur != 0 && safety++ < 100000) {
                if (set.contains(cur)) {
                    return true;
                }
                const auto* parent_hc = reg.get_component<hierarchy_component>(cur);
                cur = parent_hc ? parent_hc->parent : 0;
            }
            return false;
        }

        entity duplicate_entity_subtree(
            registry& reg,
            IEditableWorld& world,
            entity src,
            entity target_parent,
            const std::function<void(entity, std::vector<entity>&)>& gather_children_fn)
        {
            if (src == 0 || !reg.valid(src)) {
                return 0;
            }

            std::vector<entity> src_children;
            gather_children_fn(src, src_children);

            transform_component saved_local_transform{};
            const bool has_local_transform = reg.has_component<transform_component>(src);
            if (has_local_transform) {
                saved_local_transform = *reg.get_component<transform_component>(src);
            }

            entity duplicated = reg.duplicate_entity(src);
            if (target_parent != 0 && reg.valid(target_parent)) {
                world.set_entity_parent(duplicated, target_parent);
                if (has_local_transform) {
                    if (auto* dup_tc = reg.get_component<transform_component>(duplicated)) {
                        *dup_tc = saved_local_transform;
                    }
                }
            }

            for (entity child_src : src_children) {
                if (child_src != 0 && child_src != src) {
                    duplicate_entity_subtree(reg, world, child_src, duplicated, gather_children_fn);
                }
            }

            return duplicated;
        }

    } // anonymous namespace

    hierarchy_view_model::hierarchy_view_model(IEditableWorld& world, registry& reg, editor_selection_manager& selection)
        : m_world(world), m_registry(reg), m_selection_manager(selection) {
    }

    void hierarchy_view_model::gather_children_recursive(entity parent, std::vector<entity>& out_children) const {
        std::unordered_set<entity> visited;
        if (auto* hc = m_registry.get_component<hierarchy_component>(parent)) {
            entity current_child = hc->first_child;

            // Kardeş (sibling) zincirini dolaşarak tüm doğrudan çocukları topla
            int safety = 0;
            while (current_child != 0 && safety++ < 100000) {
                if (visited.contains(current_child)) {
                    break;
                }
                visited.insert(current_child);

                if (m_registry.valid(current_child)) {
                    auto* child_hc = m_registry.get_component<hierarchy_component>(current_child);
                    if (!child_hc || child_hc->parent == parent) {
                        out_children.push_back(current_child);
                    }
                    if (child_hc) {
                        current_child = child_hc->next_sibling;
                    }
                    else {
                        break;
                    }
                }
                else {
                    break;
                }
            }
        }

        // Self-healing (Kendi kendini onarma): parent'ı bu düğümü gösteren ama bağlı listeden kopmuş
        // yetim çocuk varlıklar varsa onları tespit et, bağlı listeye geri ekle ve listeye dahil et.
        for (entity candidate : m_registry.get_all_entities()) {
            if (candidate == 0 || candidate == parent || visited.contains(candidate)) {
                continue;
            }
            if (auto* cand_hc = m_registry.get_component<hierarchy_component>(candidate)) {
                if (cand_hc->parent == parent) {
                    visited.insert(candidate);
                    out_children.push_back(candidate);

                    // Bağlı listeyi de canlı olarak onar
                    auto* mut_parent_hc = m_registry.get_component<hierarchy_component>(parent);
                    if (mut_parent_hc) {
                        cand_hc->next_sibling = 0;
                        if (mut_parent_hc->first_child == 0) {
                            cand_hc->prev_sibling = 0;
                            mut_parent_hc->first_child = candidate;
                        }
                        else {
                            entity tail = mut_parent_hc->first_child;
                            int safety = 0;
                            while (tail != 0 && tail != candidate && safety++ < 100000) {
                                auto* tail_hc = m_registry.get_component<hierarchy_component>(tail);
                                if (!tail_hc || tail_hc->next_sibling == 0) {
                                    break;
                                }
                                tail = tail_hc->next_sibling;
                            }
                            if (tail != 0 && tail != candidate) {
                                if (auto* tail_hc = m_registry.get_component<hierarchy_component>(tail)) {
                                    tail_hc->next_sibling = candidate;
                                    cand_hc->prev_sibling = tail;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    hierarchy_node_data hierarchy_view_model::get_node_data(entity e) const {
        hierarchy_node_data data;
        data.id = e;
        data.is_selected = m_selection_manager.is_entity_selected(e);

        if (auto* name_comp = m_registry.get_component<name_component>(e)) {
            data.name = name_comp->name.empty() ? "Unnamed Entity" : name_comp->name;
        }
        else {
            data.name = "Entity " + std::to_string(e);
        }

        if (auto* hc = m_registry.get_component<hierarchy_component>(e)) {
            data.parent_id = hc->parent;
            gather_children_recursive(e, data.children);
            data.has_children = !data.children.empty();
        }

        return data;
    }

    std::vector<hierarchy_node_data> hierarchy_view_model::get_root_nodes() const {
        std::vector<hierarchy_node_data> root_nodes;
        const auto& all_entities = m_registry.get_all_entities();

        for (entity e : all_entities) {
            bool is_root = true;
            if (auto* hc = m_registry.get_component<hierarchy_component>(e)) {
                if (hc->parent != 0 && hc->parent != e && m_registry.valid(hc->parent)) {
                    is_root = false;
                }
            }

            if (is_root) {
                root_nodes.push_back(get_node_data(e));
            }
        }
        return root_nodes;
    }

    // ==========================================
    // MUTATION DISPATCHERS (YAZMA İŞLEMLERİ)
    // ==========================================

    void hierarchy_view_model::request_create_entity(const std::string& default_name) {
        if (m_command_stack) {
            auto cmd = std::make_unique<spawn_entity_command>(default_name);
            auto* rawCmd = cmd.get();
            m_command_stack->execute_command(std::move(cmd));
            entity e = rawCmd->get_spawned_entity();
            if (e != 0) {
                m_selection_manager.select_entity(e, false);
                start_rename(e);
            }
        } else {
            entity e = m_world.spawn_entity(default_name);
            m_selection_manager.select_entity(e, false);
            start_rename(e);
        }
    }

    void hierarchy_view_model::request_destroy_entity(entity e) {
        if (m_selection_manager.is_entity_selected(e)) {
            m_selection_manager.deselect_entity(e);
        }
        if (m_command_stack) {
            auto cmd = std::make_unique<destroy_entity_command>(e, m_registry);
            m_command_stack->execute_command(std::move(cmd));
        } else {
            m_world.destroy_entity(e);
        }
    }

    void hierarchy_view_model::request_set_parent(entity child, entity parent) {
        request_set_parent_batch({ child }, parent);
    }

    void hierarchy_view_model::request_set_parent_batch(const std::vector<entity>& raw_children, entity target_parent) {
        if (raw_children.empty()) return;

        // 1. Hedef ebeveyn geçerliliğini ve döngüsel bağımlılıkları kontrol et
        std::vector<entity> valid_children;
        std::unordered_set<entity> raw_set(raw_children.begin(), raw_children.end());

        for (entity c : raw_children) {
            if (c == 0 || !m_registry.valid(c) || c == target_parent) {
                continue;
            }
            // Eğer seçilenlerden biri başka bir seçilenin çocuğuysa, hiyerarşiyi bozmamak için
            // sadece en üst ata olanı reparent et
            if (is_ancestor_in_set(m_registry, c, raw_set)) {
                continue;
            }
            // Hedef ebeveyn taşınacak varlığın alt soyundan (descendant) geliyorsa döngü oluşur, atla
            if (target_parent != 0 && is_ancestor_in_set(m_registry, target_parent, { c })) {
                continue;
            }

            valid_children.push_back(c);
        }

        if (valid_children.empty()) {
            return;
        }

        if (m_command_stack) {
            auto cmd = std::make_unique<reparent_entity_command>(valid_children, target_parent, m_registry);
            m_command_stack->execute_command(std::move(cmd));
        } else {
            for (entity c : valid_children) {
                m_world.set_entity_parent(c, target_parent);
            }
            m_world.mark_scene_dirty();
        }
    }

    void hierarchy_view_model::request_rename_entity(entity e, const std::string& new_name) {
        if (e == 0 || !m_registry.valid(e)) {
            m_renaming_entity = 0;
            return;
        }

        std::string old_name;
        if (auto* n = m_registry.get_component<name_component>(e)) {
            old_name = n->name;
        }

        if (old_name == new_name) {
            m_renaming_entity = 0;
            return;
        }

        if (m_command_stack) {
            auto cmd = std::make_unique<rename_entity_command>(e, old_name, new_name);
            m_command_stack->execute_command(std::move(cmd));
        } else {
            if (auto* n = m_registry.get_component<name_component>(e)) {
                n->name = new_name;
                m_world.mark_scene_dirty();
            }
        }
        m_renaming_entity = 0;
    }

    void hierarchy_view_model::request_select(entity e, bool accumulate) {
        if (accumulate) {
            if (m_selection_manager.is_entity_selected(e)) {
                m_selection_manager.deselect_entity(e);
            } else {
                m_selection_manager.select_entity(e, true);
            }
        } else {
            m_selection_manager.select_entity(e, false);
        }
    }

    void hierarchy_view_model::request_copy(entity e) {
        m_clipboard_entities.clear();
        if (e != 0) {
            m_clipboard_entities.push_back(e);
        }
    }

    void hierarchy_view_model::request_paste() {
        if (m_clipboard_entities.empty()) return;

        std::unordered_set<entity> source_set(m_clipboard_entities.begin(), m_clipboard_entities.end());
        std::vector<entity> newly_created;
        newly_created.reserve(m_clipboard_entities.size());

        auto gather_fn = [this](entity p, std::vector<entity>& out) {
            this->gather_children_recursive(p, out);
        };

        for (entity src : m_clipboard_entities) {
            if (src == 0 || !m_registry.valid(src)) continue;
            if (is_ancestor_in_set(m_registry, src, source_set)) continue;

            entity target_parent = 0;
            if (const auto* hc = m_registry.get_component<hierarchy_component>(src)) {
                target_parent = hc->parent;
            }

            entity duplicated = duplicate_entity_subtree(m_registry, m_world, src, target_parent, gather_fn);
            if (duplicated != 0) {
                newly_created.push_back(duplicated);
            }
        }

        if (!newly_created.empty()) {
            m_selection_manager.clear_entity_selection();
            for (entity ne : newly_created) {
                m_selection_manager.select_entity(ne, true);
            }
            m_world.mark_scene_dirty();
        }
    }

    void hierarchy_view_model::request_duplicate(entity e) {
        if (e == 0 || !m_registry.valid(e)) return;

        entity target_parent = 0;
        if (const auto* hc = m_registry.get_component<hierarchy_component>(e)) {
            target_parent = hc->parent;
        }

        auto gather_fn = [this](entity p, std::vector<entity>& out) {
            this->gather_children_recursive(p, out);
        };

        entity duplicated = duplicate_entity_subtree(m_registry, m_world, e, target_parent, gather_fn);
        if (duplicated != 0) {
            m_selection_manager.select_entity(duplicated, false);
            m_world.mark_scene_dirty();
        }
    }

    // ==========================================
    // SELECTION (SEÇİM DURUMU)
    // ==========================================

    bool hierarchy_view_model::has_selection() const {
        return m_selection_manager.has_entity_selection();
    }

    entity hierarchy_view_model::get_selected_entity() const {
        return m_selection_manager.get_first_selected_entity();
    }

    bool hierarchy_view_model::is_selected(entity e) const {
        return m_selection_manager.is_entity_selected(e);
    }

    // ==========================================
    // RENAME STATE (YENİDEN ADLANDIRMA)
    // ==========================================

    bool hierarchy_view_model::is_renaming(entity e) const {
        return m_renaming_entity == e;
    }

    void hierarchy_view_model::start_rename(entity e) {
        m_renaming_entity = e;
        m_rename_buffer.clear();

        if (auto* name = m_registry.get_component<name_component>(e)) {
            m_rename_buffer = name->name;
        }
    }

    void hierarchy_view_model::commit_rename() {
        if (m_renaming_entity == 0) return;

        request_rename_entity(m_renaming_entity, m_rename_buffer);
        m_renaming_entity = 0;
    }

    void hierarchy_view_model::cancel_rename() {
        m_renaming_entity = 0;
        m_rename_buffer.clear();
    }

    entity hierarchy_view_model::get_renaming_entity() const {
        return m_renaming_entity;
    }

    std::string& hierarchy_view_model::get_rename_buffer() {
        return m_rename_buffer;
    }

    const std::string& hierarchy_view_model::get_rename_buffer() const {
        return m_rename_buffer;
    }

    // ==========================================
    // CLIPBOARD (PANO DURUMU)
    // ==========================================

    bool hierarchy_view_model::has_clipboard() const {
        return !m_clipboard_entities.empty();
    }

    // ==========================================
    // VALIDATION (DOĞRULAMA)
    // ==========================================

    bool hierarchy_view_model::can_copy() const {
        return has_selection();
    }

    bool hierarchy_view_model::can_duplicate() const {
        return has_selection();
    }

    bool hierarchy_view_model::can_delete() const {
        return has_selection();
    }

    bool hierarchy_view_model::can_paste() const {
        return has_clipboard();
    }

    // ==========================================
    // HIGH LEVEL ACTIONS (YÜKSEK SEVİYELİ İŞLEMLER)
    // ==========================================

    void hierarchy_view_model::request_copy_selected() {
        const auto& selected = m_selection_manager.get_selected_entities();
        m_clipboard_entities.assign(selected.begin(), selected.end());
    }

    void hierarchy_view_model::request_duplicate_selected() {
        const auto& selected = m_selection_manager.get_selected_entities();
        if (selected.empty()) return;

        std::vector<entity> to_duplicate(selected.begin(), selected.end());
        std::unordered_set<entity> source_set(to_duplicate.begin(), to_duplicate.end());
        std::vector<entity> newly_created;
        newly_created.reserve(to_duplicate.size());

        auto gather_fn = [this](entity p, std::vector<entity>& out) {
            this->gather_children_recursive(p, out);
        };

        for (entity src : to_duplicate) {
            if (src == 0 || !m_registry.valid(src)) continue;
            if (is_ancestor_in_set(m_registry, src, source_set)) continue;

            entity target_parent = 0;
            if (const auto* hc = m_registry.get_component<hierarchy_component>(src)) {
                target_parent = hc->parent;
            }

            entity duplicated = duplicate_entity_subtree(m_registry, m_world, src, target_parent, gather_fn);
            if (duplicated != 0) {
                newly_created.push_back(duplicated);
            }
        }

        if (!newly_created.empty()) {
            m_selection_manager.clear_entity_selection();
            for (entity ne : newly_created) {
                m_selection_manager.select_entity(ne, true);
            }
            m_world.mark_scene_dirty();
        }
    }

    void hierarchy_view_model::request_delete_selected() {
        const auto& selected = m_selection_manager.get_selected_entities();
        if (selected.empty()) return;

        std::vector<entity> to_delete(selected.begin(), selected.end());
        std::unordered_set<entity> selected_set(to_delete.begin(), to_delete.end());
        m_selection_manager.clear_entity_selection();

        // Eğer seçilenlerden birinin üst atası da siliniyorsa, onu ayrı komut olarak silmeye gerek yok
        // çünkü parent silindiğinde tüm çocukları da özyinelemeli olarak silinir
        std::vector<entity> roots_to_delete;
        for (entity e : to_delete) {
            if (e != 0 && m_registry.valid(e)) {
                if (!is_ancestor_in_set(m_registry, e, selected_set)) {
                    roots_to_delete.push_back(e);
                }
            }
        }

        if (roots_to_delete.empty()) return;

        if (m_command_stack) {
            if (roots_to_delete.size() == 1) {
                auto cmd = std::make_unique<destroy_entity_command>(roots_to_delete[0], m_registry);
                m_command_stack->execute_command(std::move(cmd));
            } else {
                auto compound = std::make_unique<compound_command>();
                for (entity e : roots_to_delete) {
                    compound->add_command(std::make_unique<destroy_entity_command>(e, m_registry));
                }
                m_command_stack->execute_command(std::move(compound));
            }
        } else {
            for (entity e : roots_to_delete) {
                m_world.destroy_entity(e);
            }
        }
    }

    const std::unordered_set<entity>& hierarchy_view_model::get_selected_entities() const {
        return m_selection_manager.get_selected_entities();
    }

    bool hierarchy_view_model::can_undo() const {
        return m_command_stack && m_command_stack->can_undo();
    }

    bool hierarchy_view_model::can_redo() const {
        return m_command_stack && m_command_stack->can_redo();
    }

    void hierarchy_view_model::request_undo() {
        if (can_undo()) {
            m_command_stack->undo();
        }
    }

    void hierarchy_view_model::request_redo() {
        if (can_redo()) {
            m_command_stack->redo();
        }
    }

    void hierarchy_view_model::request_create_camera() {
        entity e = m_world.spawn_entity("Camera");
        if (!m_registry.has_component<camera_component>(e)) {
            m_registry.add_component<camera_component>(e);
        }
        m_selection_manager.select_entity(e, false);
        m_world.mark_scene_dirty();
    }

    void hierarchy_view_model::request_create_light(light_type type) {
        const char* defaultName = "Point Light";
        if (type == light_type::directional) defaultName = "Directional Light";
        else if (type == light_type::spot)   defaultName = "Spot Light";

        entity e = m_world.spawn_entity(defaultName);
        if (!m_registry.has_component<light_component>(e)) {
            m_registry.add_component<light_component>(e);
        }
        if (auto* lc = m_registry.get_component<light_component>(e)) {
            lc->type = type;
        }
        m_selection_manager.select_entity(e, false);
        m_world.mark_scene_dirty();
    }

} // namespace vesta
