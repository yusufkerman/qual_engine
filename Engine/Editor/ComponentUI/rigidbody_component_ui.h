/**
 * @file rigidbody_component_ui.h
 * @brief Katı cisim bileşeni (rigidbody_component) için Jolt Physics tabanlı editör arayüzü.
 *
 * Bu dosya; sahnedeki varlıkların hareket türlerinin (Static, Kinematic, Dynamic), kütle,
 * sürtünme katsayılarının manipüle edilmesini ve runtime (PLAY) modunda bileşen kaldırıldığında
 * Jolt Physics belleğinden ham gövdelerin (Body) güvenle temizlenmesini yönetir.
 */

#pragma once
#include "component_ui_registry.h"
#include "../../ECS/registry.h"
#include "../../Engine/engine_context.h"
#include <imgui.h>

 /**
  * @brief Seçili varlığın (entity) katı cisim (rigidbody) verilerini ImGui üzerinde çizip günceller.
  *
  * @param e Üzerinde işlem yapılan ve arayüzü çizilecek olan ECS varlığı (Entity).
  * @param reg ECS veri tabanı ve bileşen yöneticisi (Registry) referansı.
  * @param engineCtx Motorun genel çalışma zamanı bağlamı (EngineContext) - Fizik motoru erişimi için.
  * @param editorCtx Editörün panel odakları ve arayüz durumu (EditorContext).
  */
inline void render_rigidbody_ui(entity e, registry& reg, EngineContext& engineCtx, EditorContext& editorCtx) {
    auto* rb = reg.get_component<rigidbody_component>(e);
    if (!rb) return;

    // ---------------------------------------------------------
    // JOLT PHYSICS BODY ID VE DURUM KONTROLÜ
    // ---------------------------------------------------------
    // Eğer geçerli bir Jolt ID'si varsa simülasyon aktiftir (Runtime / PLAY modu)
    if (!rb->body_id.IsInvalid()) {
        ImGui::Text("Jolt Body ID: %u", rb->body_id.GetIndex());
    }
    else {
        // Editör modundayken fizik motoru henüz ilklendirilmediği için kullanıcıyı bilgilendiriyoruz
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Status: Editor Mode (Fizik Pasif)");
    }

    ImGui::Separator();

    // ---------------------------------------------------------
    // 1. HAREKET TÜRÜ SEÇİMİ (MOTION TYPE)
    // ---------------------------------------------------------
    const char* motionNames[] = { "Static", "Kinematic", "Dynamic" };
    int motionIndex = (int)rb->motion;

    if (ImGui::Combo("Motion Type", &motionIndex, motionNames, 3)) {
        rb->motion = (motion_type)motionIndex;
    }

    // ---------------------------------------------------------
    // 2. KÜTLE VE SÜRTÜNME AYARLARI
    // ---------------------------------------------------------
    // Statik nesnelerin (Duvar, zemin vb.) kütlesi fizik kuralları gereği sıfır olmalıdır.
    // Kullanıcının statik gövdelere yanlışlıkla kütle girmesini engellemek için arayüzü kilitliyoruz.
    if (rb->motion == motion_type::static_body) {
        ImGui::BeginDisabled();
        float dummyMass = 0.0f;
        ImGui::DragFloat("Mass (Kg)", &dummyMass, 0.0f);
        ImGui::EndDisabled();
    }
    else {
        // Dinamik ve kinematik nesneler için 0.05 hassasiyetle kütle ayarı
        ImGui::DragFloat("Mass (Kg)", &rb->mass, 0.05f, 0.01f, 10000.0f, "%.2f");
    }

    // Fiziksel yüzey sürtünme katsayısı (0.0 ile 1.0 arasında sınırlandırılmış)
    ImGui::SliderFloat("Friction", &rb->friction, 0.0f, 1.0f, "%.2f");

    ImGui::Separator();

    // ---------------------------------------------------------
    // 3. BİLEŞEN KALDIRMA VE RUTİN FİZİK TEMİZLİĞİ
    // ---------------------------------------------------------
    if (ImGui::Button("Remove Rigidbody")) {
        // Eğer oyun çalışıyorsa (PLAY modu) ve Jolt üzerinde aktif bir gövde varsa,
        // bellek sızıntısını ve fizik motorunun çökmesini engellemek için gövdeyi Jolt dünyasından siliyoruz.
        if (engineCtx.editor.mode == EngineMode::PLAY && !rb->body_id.IsInvalid()) {
            auto* jolt_system = engineCtx.physicsSystem.get_jolt_system();
            jolt_system->GetBodyInterface().RemoveBody(rb->body_id);
            jolt_system->GetBodyInterface().DestroyBody(rb->body_id);
        }
        // Fizik dünyasından temizlenen bileşeni ECS registry üzerinden de güvenle kaldırıyoruz
        reg.remove_component<rigidbody_component>(e);
    }
}

/**
 * @brief Belirtilen varlığa (entity) eğer sahip değilse varsayılan bir katı cisim bileşeni ekler.
 *
 * @param e Katı cisim bileşeninin ekleneceği ECS varlığı.
 * @param reg ECS veri tabanı (Registry) referansı.
 */
inline void add_rigidbody_component(entity e, registry& reg, EngineContext& engineCtx, EditorContext&) {
    if (!reg.has_component<rigidbody_component>(e)) {
        // Motorun orijinal varsayılan değerleri: Dinamik hareket türü, 1.0 Kg kütle ve 0.5 sürtünme katsayısı
        reg.add_component<rigidbody_component>(e, rigidbody_component{ motion_type::dynamic_body, 1.0f, 0.5f });
    }
}

/**
 * @brief Katı Cisim (Rigidbody) arayüz alt sistemini merkezi UI kayıt yöneticisine enjekte eder.
 *
 * Şablon (template) bağımlılıklarını izole etmek için runtime tabanlı veri haritası kaydını gerçekleştirir.
 */
inline void register_rigidbody_ui_system() {
    ComponentUIInfo info;
    info.name = "Rigidbody";
    info.render_fn = render_rigidbody_ui;
    info.add_fn = add_rigidbody_component;

    // Tip gizleme (type-erasure) işlemi için has_component kontrol lambdası
    info.has_fn = [](entity e, const registry& reg) -> bool {
        return reg.has_component<rigidbody_component>(e);
        };

    // Rigidbody bileşenini ECS'den güvenle kaldıran remove_fn lambdası
    info.remove_fn = [](entity e, registry& reg) {
        reg.remove_component<rigidbody_component>(e);
        };

    // Benzersiz tip imzasını (hash_code) anahtar olarak kullanarak sisteme kayıt işlemini tamamlıyoruz
    component_ui_registry::register_component(typeid(rigidbody_component).hash_code(), info);
}