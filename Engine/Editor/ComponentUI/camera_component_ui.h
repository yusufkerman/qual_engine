/**
 * @file camera_component_ui.h
 * @brief Kamera bileşeni (camera_component) için editör arayüzü yönetim sistemi.
 * * Bu dosya, ECS mimarisindeki kamera bileşenlerinin ImGui panellerinde dinamik olarak
 * çizilmesi, ilklendirilmesi ve merkezi UI sistemine kayıt edilmesini sağlar.
 */

#pragma once
#include "component_ui_registry.h"
#include "../../ECS/registry.h"
#include <imgui.h>

 /**
  * @brief Seçili varlığın (entity) kamera bileşeni verilerini ImGui üzerinde çizip günceller.
  * * @param e Üzerinde işlem yapılan ve arayüzü çizilecek olan ECS varlığı (Entity).
  * @param reg ECS veri tabanı ve bileşen yöneticisi (Registry) referansı.
  * @param engineCtx Motorun genel çalışma zamanı bağlamı ve alt sistemleri (EngineContext).
  * @param editorCtx Editörün panel odakları ve arayüz durumu (EditorContext).
  */
inline void render_camera_ui(entity e, registry& reg, EngineContext& engineCtx, EditorContext& editorCtx) {
    // Görsel bütünlük için katlanabilir bir başlık (Collapsing Header) oluşturuyoruz
    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {

        // Varlığa ait kamera bileşeninin ham göstergesini (pointer) güvenli bir şekilde çekiyoruz
        auto* c = reg.get_component<camera_component>(e);
        if (c) {
            // Görüş Açısı (Field of View) ayarı - 1'er birimlik adımlarla kaydırılabilir
            ImGui::DragFloat("FOV", &c->fov, 1.0f);

            // Yakın Kırpma Düzlemi (Near Clip Plane) ayarı - Hassaslık için 0.01'lik adımlarla kaydırılabilir
            ImGui::DragFloat("Near", &c->nearPlane, 0.01f);

            // Uzak Kırpma Düzlemi (Far Clip Plane) ayarı - 1'er birimlik adımlarla kaydırılabilir
            ImGui::DragFloat("Far", &c->farPlane, 1.0f);
        }
    }
}

/**
 * @brief Belirtilen varlığa (entity) eğer sahip değilse yeni bir kamera bileşeni ekler.
 * * "Add Component" menüsünden tetiklendiğinde çalışır ve mükerrer bileşen eklenmesini engeller.
 * * @param e Bileşenin ekleneceği ECS varlığı.
 * @param reg ECS veri tabanı (Registry) referansı.
 */
inline void add_camera_component(entity e, registry& reg, EngineContext&, EditorContext&) {
    // Varlığın zaten bir kamerası olup olmadığını kontrol ediyoruz (Mükerrerlik koruması)
    if (!reg.has_component<camera_component>(e)) {
        // Varsayılan kurucu fonksiyon (constructor) değerleriyle bileşeni ekliyoruz
        reg.add_component<camera_component>(e);
    }
}

/**
 * @brief Kamera bileşeni arayüz alt sistemini merkezi UI kayıt yöneticisine kaydeder.
 * * `ui_manager::init` veya `editor::init` fonksiyonu içerisinden bir kez çağrılması gerekir.
 * Metot, şablon (template) bağımlılığını ezmek için runtime lambda fonksiyonları üretir.
 */
inline void register_camera_ui_system() {
    // Merkezi sisteme aktarılacak olan arayüz meta bilgilerini ve fonksiyon göstergelerini hazırlıyoruz
    ComponentUIInfo info;

    // Editör menülerinde ve panellerinde görünecek olan okunaklı isim
    info.name = "Camera";

    // ImGui çizim fonksiyonunun atanması
    info.render_fn = render_camera_ui;

    // Bileşen ekleme fonksiyonunun atanması
    info.add_fn = add_camera_component;

    // Tipi gizlenmiş (type-erased) registry için varlık bileşen kontrol lambdasının tanımlanması
    info.has_fn = [](entity e, const registry& reg) -> bool {
        return reg.has_component<camera_component>(e);
        };

    // Kamera bileşeninin benzersiz tip kodunu (type-hash) anahtar olarak kullanarak merkezi sisteme kayıt ediyoruz
    component_ui_registry::register_component(typeid(camera_component).hash_code(), info);
}