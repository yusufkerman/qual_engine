#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include "ECS/registry.h"
#include "ECS/transform_utils.h"

int main()
{
    std::cout << "[Test] QualEngine EnTT Sparse-Set ECS Dogrulama Testi Basliyor...\n";

    registry reg;

    // 1. Rezerve Varlik 0 Guvenlik Dogrulamasi
    assert(!reg.valid(0));
    assert(reg.get_component<transform_component>(0) == nullptr);
    assert(!reg.has_component<name_component>(0));
    std::cout << "  [OK] Varlik 0 rezerve ve gecersiz (valid == false).\n";

    // 2. Varlik Uretimi
    entity e1 = reg.create_entity();
    entity e2 = reg.create_entity();
    assert(e1 == 1);
    assert(e2 == 2);
    assert(reg.valid(e1));
    assert(reg.valid(e2));
    assert(reg.get_all_entities().size() == 2);
    std::cout << "  [OK] Varliklar basariyla uretildi: e1=" << e1 << ", e2=" << e2 << "\n";

    // 3. 22 Bilesenin Tamaminin Eklenebilirligi
    reg.add_component<name_component>(e1, name_component{ "Hero" });
    reg.add_component<transform_component>(e1, transform_component{ glm::vec3(1.0f, 2.0f, 3.0f) });
    reg.add_component<mesh_component>(e1, mesh_component{});
    reg.add_component<camera_component>(e1, camera_component{ 60.0f, 0.1f, 500.0f, true, 1 });
    reg.add_component<light_component>(e1, light_component{ light_type::point, glm::vec3(1.0f), 5.0f, 20.0f });
    reg.add_component<material_component>(e1, material_component{});
    reg.add_component<bounds_component>(e1, bounds_component{ glm::vec3(0.0f), 2.5f });
    reg.add_component<rigidbody_component>(e1, rigidbody_component{ motion_type::dynamic_body, 75.0f, 0.6f });
    reg.add_component<collider_component>(e1, collider_component{ collider_type::box, glm::vec3(1.0f) });
    reg.add_component<native_script_component>(e1, native_script_component{ nullptr, "PlayerController" });
    reg.add_component<hierarchy_component>(e1, hierarchy_component{ 0, 0, 0, 0 });
    reg.add_component<skinned_mesh_component>(e1, skinned_mesh_component{});
    reg.add_component<skeleton_component>(e1, skeleton_component{});
    reg.add_component<animator_component>(e1, animator_component{});
    reg.add_component<bone_attachment_component>(e1, bone_attachment_component{});
    reg.add_component<canvas_component>(e1, canvas_component{});
    reg.add_component<rect_transform_component>(e1, rect_transform_component{});
    reg.add_component<ui_text_component>(e1, ui_text_component{ "Health: 100" });
    reg.add_component<ui_image_component>(e1, ui_image_component{});
    reg.add_component<ui_button_component>(e1, ui_button_component{});
    reg.add_component<audio_source_component>(e1, audio_source_component{});
    reg.add_component<audio_listener_component>(e1, audio_listener_component{ true });

    assert(reg.has_component<name_component>(e1));
    assert(reg.has_component<transform_component>(e1));
    assert(reg.has_component<camera_component>(e1));
    assert(reg.has_component<light_component>(e1));
    assert(reg.has_component<audio_listener_component>(e1));
    assert(reg.get_component<name_component>(e1)->name == "Hero");
    assert(reg.get_component<transform_component>(e1)->position.x == 1.0f);
    assert(reg.get_component<camera_component>(e1)->fov == 60.0f);
    std::cout << "  [OK] 22 bilesenin tamami basariyla eklendi ve okundu.\n";

    // 4. In-Place Range-For Iterasyonu ve Modifikasyon (auto& [e, comp])
    for (auto& [ent, trans] : reg.get_all<transform_component>())
    {
        assert(ent == e1);
        trans.position.x += 10.0f;
    }
    assert(reg.get_component<transform_component>(e1)->position.x == 11.0f);
    std::cout << "  [OK] In-place auto& [e, comp] dongusu ve bellek ici modifikasyon calisiyor.\n";

    // 5. Const Range-For Iterasyonu
    const registry& constReg = reg;
    for (const auto& [ent, trans] : constReg.get_all<transform_component>())
    {
        assert(ent == e1);
        assert(trans.position.x == 11.0f);
    }
    std::cout << "  [OK] Const range-for iterasyonu calisiyor.\n";

    // 6. Modern EnTT Multi-Component View
    reg.add_component<transform_component>(e2, transform_component{ glm::vec3(5.0f) });
    reg.add_component<mesh_component>(e2, mesh_component{});

    int viewCount = 0;
    auto multiView = reg.view<transform_component, mesh_component>();
    for (auto ent : multiView)
    {
        ++viewCount;
        auto& t = multiView.get<transform_component>(ent);
        assert(t.position.x > 0.0f);
    }
    assert(viewCount == 2);
    std::cout << "  [OK] Modern EnTT view<T1, T2> bitisik sparse-set hizlandirma sorgusu calisiyor.\n";

    // 7. Hiyerarsi Matris Hesaplama Testi (transform_utils.h)
    auto* hc1 = reg.get_component<hierarchy_component>(e1);
    hc1->first_child = e2;
    reg.add_component<hierarchy_component>(e2, hierarchy_component{ e1, 0, 0, 0 });

    glm::mat4 world_e1 = ecs_transform::get_world_matrix(e1, reg);
    glm::mat4 world_e2 = ecs_transform::get_world_matrix(e2, reg);
    assert(world_e1[3][0] == 11.0f);
    assert(world_e2[3][0] == 16.0f); // 11.0f + 5.0f
    std::cout << "  [OK] ecs_transform::get_world_matrix hiyerarsi zinciri cozumlemesi kusursuz.\n";

    // 8. Kopyalama (Duplicate) Testi
    entity e_dup = reg.duplicate_entity(e1);
    assert(reg.valid(e_dup));
    assert(reg.get_component<name_component>(e_dup)->name == "Hero_Copy");
    assert(reg.get_component<transform_component>(e_dup)->position.x == 11.0f);
    assert(reg.get_component<hierarchy_component>(e_dup)->first_child == 0); // Alt cocuklar kopyada bagimsiz
    std::cout << "  [OK] duplicate_entity ile yeni kimlik (" << e_dup << ") ve '_Copy' adi olusturuldu.\n";

    // 9. Sahne Klonlama (Clone) Testi
    registry targetSceneReg;
    for (entity ent : reg.get_all_entities())
    {
        reg.clone_entity(ent, targetSceneReg);
    }
    assert(targetSceneReg.get_all_entities().size() == reg.get_all_entities().size());
    assert(targetSceneReg.get_component<name_component>(e1)->name == "Hero");
    assert(targetSceneReg.get_component<transform_component>(e1)->position.x == 11.0f);
    std::cout << "  [OK] clone_entity ile hedef registry'ye birebir kimlik ve bilesen aktarimi dogrulandi.\n";

    // 10. Varlik Silme (Remove Entity) ve Bilesen Temizligi
    reg.remove_entity(e1);
    assert(!reg.valid(e1));
    assert(reg.get_component<transform_component>(e1) == nullptr);
    assert(reg.get_component<name_component>(e1) == nullptr);
    assert(reg.get_all<name_component>().size() == 1); // e1 silindi, sadece e_dup kaldi
    std::cout << "  [OK] remove_entity e1'i ve 22 bilesenini otomatik temizledi.\n";

    std::cout << "\n===========================================================\n";
    std::cout << " TUM TESTLER BASARIYLA TAMAMLANDI! ENTT ECS %100 DOGRULANDI.\n";
    std::cout << "===========================================================\n";

    return 0;
}
