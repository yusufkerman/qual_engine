#pragma once
#include "component_ui_registry.h"
#include "../../ECS/registry.h" // <-- Derleyici burada registry metotlarını net görüyor
#include <imgui.h>

inline void render_name_ui(entity e, registry& reg, EngineContext& engineCtx, EditorContext& editorCtx) {
    auto* n = reg.get_component<name_component>(e);
    if (n) {
        ImGui::Text("Name: %s", n->name.c_str());
        ImGui::Separator();
    }
}

inline void add_name_component(entity e, registry& reg, EngineContext&, EditorContext&) {
    if (!reg.has_component<name_component>(e)) {
        reg.add_component<name_component>(e, name_component{ "Entity" });
    }
}

inline void register_name_ui_system() {
    ComponentUIInfo info;
    info.name = "Name";
    info.render_fn = render_name_ui;
    info.add_fn = add_name_component;
    info.has_fn = [](entity e, const registry& reg) -> bool {
        return reg.has_component<name_component>(e);
        };

    // Yeni non-template doğrudan kayıt fonksiyonumuzu çağırıyoruz
    component_ui_registry::register_component(typeid(name_component).hash_code(), info);
}