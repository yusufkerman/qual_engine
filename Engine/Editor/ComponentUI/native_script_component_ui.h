/**
 * @file native_script_component_ui.h
 * @brief Yerel betik bileşeni için dinamik editör arayüz yönetim dosyası.
 */
#pragma once
#include "component_ui_registry.h"
#include "../../ECS/registry.h"
#include "../../Engine/Scripting/script_factory.h" // Fabrikaya erişim için
#include <imgui.h>
#include <vector>
#include <string>

inline void render_native_script_ui(entity e, registry& reg, EngineContext&, EditorContext&) {
    auto* nsc = reg.get_component<native_script_component>(e);
    if (!nsc) return;

    // Fabrikadaki tüm kayıtlı script isimlerini çekiyoruz (Örn: "player_controller", "camera_controller")
    std::vector<std::string> available_scripts = script_factory::get_all_script_names();

    std::string current_selection = nsc->script_name.empty() ? "None" : nsc->script_name;

    // ImGui Combo Box (Açılır Liste) başlatıyoruz
    if (ImGui::BeginCombo("Script Class", current_selection.c_str())) {

        // Boşaltmak stands için bir "None" seçeneği
        if (ImGui::Selectable("None", current_selection == "None")) {
            nsc->script_name = "None";
            if (nsc->instance && nsc->bind_destroy) {
                nsc->bind_destroy(nsc); // Varsa eski script nesnesini RAM'den sil
            }
        }

        // Kayıtlı tüm scriptleri listele
        for (const auto& script_name : available_scripts) {
            bool is_selected = (current_selection == script_name);

            if (ImGui::Selectable(script_name.c_str(), is_selected)) {
                // 🚀 Kullanıcı listeden bir script seçtiği an canlı BIND işlemi tetikleniyor!
                if (nsc->instance && nsc->bind_destroy) {
                    nsc->bind_destroy(nsc); // Önce eski scripti temizle
                }

                // Fabrikadan bu isimdeki C++ sınıfını entity'ye bağla
                script_factory::instantiate_script(script_name, e, reg);
            }

            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
}

inline void add_native_script_component(entity e, registry& reg, EngineContext&, EditorContext&) {
    if (!reg.has_component<native_script_component>(e)) {
        auto& nsc = reg.add_component<native_script_component>(e);
        nsc.script_name = "None"; // İlk eklendiğinde içi boş (None) başlasın, kullanıcı seçsin
    }
}

inline void register_native_script_ui_system() {
    ComponentUIInfo info;
    info.name = "Native Script";
    info.render_fn = render_native_script_ui;
    info.add_fn = add_native_script_component;
    info.has_fn = [](entity e, const registry& reg) -> bool {
        return reg.has_component<native_script_component>(e);
        };

    info.remove_fn = [](entity e, registry& reg) {
        auto* nsc = reg.get_component<native_script_component>(e);
        if (nsc && nsc->instance && nsc->bind_destroy) {
            nsc->bind_destroy(nsc); // Önce RAM'deki script nesnesini (player_controller vb.) siler
        }
        reg.remove_component<native_script_component>(e); // Sonra bileşeni ECS'den koparır
        };

    component_ui_registry::register_component(typeid(native_script_component).hash_code(), info);
}