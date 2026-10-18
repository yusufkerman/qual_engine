#pragma once

/**
 * @file native_script_component_ui.h
 * @brief Yerel C++ betik bileşeni (native_script_component) için dinamik arayüz ve yaşam döngüsü yönetim sistemi.
 */

#include "../../Services/component_ui_registry.h"
#include "../../Services/component_operation_registry.h"
#include "../../ECS/registry.h"
#include "../../Engine/Scripting/script_factory.h"
#include <imgui.h>
#include <vector>
#include <string>

 /**
  * @brief Seçili varlığın C++ betik sınıfı atamasını ImGui açılır listesi ile yönetir.
  * @param e Betik bağlanacak varlık.
  * @param reg İlgili ECS kayıt defteri referansı.
  */
inline void render_native_script_ui(entity e, registry& reg) {
    auto* nsc = reg.get_component<native_script_component>(e);
    if (!nsc) return;

    // Fabrikadaki tüm kayıtlı script isimlerini sorgula
    std::vector<std::string> available_scripts = script_factory::get_all_script_names();
    std::string current_selection = nsc->script_name.empty() ? "None" : nsc->script_name;

    if (ImGui::BeginCombo("Script Class", current_selection.c_str())) {

        // Betik bağlantısını koparma seçeneği
        if (ImGui::Selectable("None", current_selection == "None")) {
            nsc->script_name = "None";
            if (nsc->instance && nsc->bind_destroy) {
                nsc->bind_destroy(nsc);
            }
        }

        // Kayıtlı tüm sınıfları listele
        for (const auto& script_name : available_scripts) {
            bool is_selected = (current_selection == script_name);

            if (ImGui::Selectable(script_name.c_str(), is_selected)) {
                // Yeni betik bağlanırken eski örneği bellekten temizle
                if (nsc->instance && nsc->bind_destroy) {
                    nsc->bind_destroy(nsc);
                }

                // Seçilen C++ sınıfını entity'ye bağla ve başlat
                script_factory::instantiate_script(script_name, e, reg);
            }

            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
}

/**
 * @brief Bir varlığa varsayılan native_script_component ekler.
 */
inline void add_native_script_component(entity e, registry& reg) {
    if (!reg.has_component<native_script_component>(e)) {
        auto& nsc = reg.add_component<native_script_component>(e);
        nsc.script_name = "None";
    }
}

/**
 * @brief Yerel betik bileşeni için arayüz ve bellek güvenli yaşam döngüsü operasyonlarını kaydeder.
 */
inline void register_native_script_ui_system() {
    size_t comp_hash = typeid(native_script_component).hash_code();

    // 1. ECS Operasyon Kaydı (Bellek temizliği garantili)
    vesta::ComponentOperationInfo logic;
    logic.add_fn = add_native_script_component;

    logic.has_fn = [](entity e, registry& reg) -> bool {
        return reg.has_component<native_script_component>(e);
        };

    logic.remove_fn = [](entity e, registry& reg) {
        auto* nsc = reg.get_component<native_script_component>(e);
        if (nsc && nsc->instance && nsc->bind_destroy) {
            nsc->bind_destroy(nsc); // Bellekteki C++ nesne örneğini yok et
        }
        reg.remove_component<native_script_component>(e);
        };

    vesta::component_operation_registry::register_component(comp_hash, logic);

    // 2. UI Çizim Kaydı
    vesta::ComponentUIInfo ui;
    ui.display_name = "Native Script";
    ui.category = "Scripting";
    ui.render_fn = [](entity e, registry& reg) {
        render_native_script_ui(e, reg);
        };

    vesta::component_ui_registry::register_component(comp_hash, ui);
}