#pragma once

/**
 * @file mesh_collider_component_ui.h
 * @brief Ağ çarpıştırıcı bileşeni (mesh_collider_component) için VESTA editör arayüzü ve operasyon kayıt sistemi.
 */

#include "../Vesta/Services/component_ui_registry.h"
#include "../../Services/component_operation_registry.h"
#include "../../../ECS/registry.h"
#include "../../../ECS/components.h"
#include "../../../Systems/mesh_collider_baker.h"
#include "../../../Engine/Assets/aria.h"
#include <imgui.h>

namespace
{
    inline aria::system* s_mesh_col_aria = nullptr;
}

/**
 * @brief Seçili varlığın Mesh Collider ayarlarını, çözünürlük modunu ve hiyerarşi istatistiklerini çizer.
 */
inline void render_mesh_collider_ui(entity e, registry& reg)
{
    auto* col = reg.get_component<mesh_collider_component>(e);
    if (!col) return;

    bool needsBake = false;

    // 1. Çözünürlük ve Sadeleştirme Modu Seçimi
    const char* resolutionNames[] = {
        "Full (1:1 - Tum Ucgenler / Kusursuz Detay)",
        "High (~75% - Hafif Sadelestirme)",
        "Medium (~40% - Standart / Onerilen)",
        "Low (~20% - Yuksek Performans)",
        "Convex Hull (Dis Bukey Kabuk)"
    };
    int resIndex = static_cast<int>(col->resolution);

    if (ImGui::Combo("Resolution", &resIndex, resolutionNames, IM_ARRAYSIZE(resolutionNames)))
    {
        col->resolution = static_cast<mesh_collider_resolution>(resIndex);
        if (col->resolution == mesh_collider_resolution::convex_hull)
        {
            col->is_convex = true;
        }
        needsBake = true;
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Mesh collider detay seviyesi.\nFull: Orijinal geometriyi 1:1 korur.\nMedium/Low: Spatial Grid Clustering ile ucgen sayisini azaltarak yuksek fizik performansi saglar.");
    }

    ImGui::Spacing();

    // 2. Convex ve Trigger Bayrakları
    if (ImGui::Checkbox("Is Convex", &col->is_convex))
    {
        needsBake = true;
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Dinamik (Dynamic Body) rigidbody nesneleri icin mesh collider'in dis bukey (Convex Hull) olmasi zorunludur. Statik binalar ve zeminler icin kapali tutulabilir.");
    }

    ImGui::SameLine();
    ImGui::Checkbox("Is Trigger", &col->is_trigger);
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Kati fiziksel carpisma yerine sadece temas (trigger) olaylarini algilayan sensor moduna gecirir.");
    }

    ImGui::Separator();

    // 3. Konum Ötelemesi (Center Offset)
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Center Offset");
    if (ImGui::DragFloat3("Offset", &col->offset.x, 0.05f, -100.0f, 100.0f, "%.2f"))
    {
        needsBake = true;
    }

    ImGui::Separator();

    // 4. Hiyerarşik Geometri ve Pişirme (Bake) İstatistikleri
    if (col->is_dirty || col->cooked_vertices.empty() || needsBake)
    {
        qual::physics::mesh_collider_baker::bake(e, reg, s_mesh_col_aria, *col);
    }

    ImGui::TextColored(ImVec4(0.35f, 0.75f, 1.0f, 1.0f), "Submesh Hiyerarsisi:");
    ImGui::BulletText("Bulunan Alt Ag (Submesh): %u", col->submesh_count);

    if (col->submesh_count > 1)
    {
        ImGui::TextColored(ImVec4(0.35f, 0.85f, 0.45f, 1.0f), "Alt cocuk nesnelerdeki %u submesh otomatik birlestirildi.", col->submesh_count);
    }

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.0f), "Geometri ve Sadelestirme:");
    ImGui::BulletText("Orijinal : %u kose, %u ucgen", col->original_vertex_count, col->original_triangle_count);
    ImGui::BulletText("Collider : %u kose, %u ucgen", col->cooked_vertex_count, col->cooked_triangle_count);

    if (col->original_triangle_count > 0 && col->cooked_triangle_count < col->original_triangle_count)
    {
        const float reduction = (1.0f - static_cast<float>(col->cooked_triangle_count) / static_cast<float>(col->original_triangle_count)) * 100.0f;
        ImGui::TextColored(ImVec4(0.40f, 0.90f, 0.50f, 1.0f), "Performans Kazanci: -%.1f%% ucgen", reduction);
    }

    ImGui::Spacing();

    // 5. Yeniden Pişirme Butonu
    if (ImGui::Button("Rebuild / Bake Collider", ImVec2(-1.0f, 26.0f)))
    {
        col->is_dirty = true;
        qual::physics::mesh_collider_baker::bake(e, reg, s_mesh_col_aria, *col);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Modeldeki veya cocuk nesnelerdeki degisiklikleri yeniden okuyup carpistirici geometrisini sifirdan olusturur.");
    }
}

/**
 * @brief Mesh Collider bileşeni için UI çizici ve ECS operasyon rutinlerini sisteme kaydeder.
 */
inline void register_mesh_collider_ui_system(aria::system* ariaSys)
{
    s_mesh_col_aria = ariaSys;
    const size_t comp_hash = typeid(mesh_collider_component).hash_code();

    // 1. UI Çizim Kaydı
    vesta::ComponentUIInfo ui_info;
    ui_info.display_name = "Mesh Collider";
    ui_info.category = "Physics";
    ui_info.render_fn = render_mesh_collider_ui;
    vesta::component_ui_registry::register_component(comp_hash, ui_info);

    // 2. ECS Operasyon Kaydı
    vesta::ComponentOperationInfo op_info;

    op_info.add_fn = [](entity e, registry& reg) {
        if (!reg.has_component<mesh_collider_component>(e)) {
            auto& col = reg.add_component<mesh_collider_component>(e, mesh_collider_component{});
            qual::physics::mesh_collider_baker::bake(e, reg, s_mesh_col_aria, col);
        }
    };

    op_info.has_fn = [](entity e, registry& reg) -> bool {
        return reg.has_component<mesh_collider_component>(e);
    };

    op_info.remove_fn = [](entity e, registry& reg) {
        reg.remove_component<mesh_collider_component>(e);
    };

    vesta::component_operation_registry::register_component(comp_hash, op_info);
}

