/**
 * @file component_ui_registry.h
 * @brief ECS bileşenleri için veri odaklı (data-driven) merkezi arayüz kayıt yöneticisi.
 * * Bu dosya, bileşenlerin UI çizim ve yönetim mantıklarını tip gizleme (type-erasure)
 * yöntemiyle soyutlar. Böylece `inspector_panel` dosyalarının somut bileşen türlerine
 * (Transform, Mesh vb.) olan bağımlılığını (compile-time bağımlılık zincirini) tamamen koparır.
 */

#pragma once
#include <unordered_map>
#include <functional>
#include "../../ECS/entity.h"

 // Forward declarations (Ön Bildirimler) - Sıfır Derleme Bağımlılığı kuralı
class registry;
struct EngineContext;
struct EditorContext;

/**
 * @brief Bir bileşenin ImGui panelinde çizilmesini sağlayan fonksiyon türü (Callback).
 * @note std::function sarmalayıcısı, forward-declared sınıfların üye şablon metot çağrılarını runtime'a öteler.
 */
using ComponentRenderFn = std::function<void(entity, registry&, EngineContext&, EditorContext&)>;

/**
 * @brief Bir varlığa (entity) ilgili bileşenin varsayılan değerlerle eklenmesini sağlayan fonksiyon türü.
 */
using ComponentAddFn = std::function<void(entity, registry&, EngineContext&, EditorContext&)>;

/**
 * @brief Bir varlığın ilgili bileşene sahip olup olmadığını sorgulayan fonksiyon türü.
 */
using ComponentHasFn = std::function<bool(entity, const registry&)>;

/**
 * @brief Bir bileşene ait editör arayüzü meta verilerini ve fonksiyon işaretçilerini tutan veri yapısı.
 */
struct ComponentUIInfo {
    const char* name;             /**< Editör menülerinde ve başlıklarında görünecek okunaklı isim. */
    ComponentRenderFn render_fn;  /**< Bileşenin özelliklerini ImGui'de çizen fonksiyon sarmalı. */
    ComponentAddFn add_fn;        /**< "Add Component" popup'ından çağrılan ekleme fonksiyonu sarmalı. */
    ComponentHasFn has_fn;        /**< `registry::has_component` çağrısını gizleyen sorgu lambdası. */
};

/**
 * @brief Tüm bileşen arayüz sistemlerinin dinamik olarak kaydedildiği ve sorgulandığı merkezi statik sınıf.
 */
class component_ui_registry {
public:

    /**
     * @brief Yeni bir bileşen arayüz sistemini merkezi veri tabanına kaydeder.
     * * @param type_hash Kaydedilecek bileşenin `typeid(T).hash_code()` ile üretilen benzersiz tip anahtarı.
     * @param info Bileşene ait meta verileri ve fonksiyon işaretçilerini barındıran struct yapısı.
     */
    static void register_component(size_t type_hash, const ComponentUIInfo& info) {
        get_components()[type_hash] = info;
    }

    /**
     * @brief Kayıtlı tüm bileşen arayüz bilgilerini salt-okunur (const) referans olarak döndürür.
     * * `inspector_panel` bu fonksiyonu kullanarak tüm bileşenleri tek bir döngüde dinamik olarak işler.
     * * @return const std::unordered_map<size_t, ComponentUIInfo>& Kayıtlı bileşen veri haritası.
     */
    static const std::unordered_map<size_t, ComponentUIInfo>& get_registry() {
        return get_components();
    }

private:
    /**
     * @brief Statik veri tabanını (unordered_map) Meyers' Singleton kalıbıyla güvenli bir şekilde oluşturur.
     * * Static initialization order fiasco (statik ilklendirme sırası karmaşası) riskini
     * engellemek için harita, fonksiyon içi yerel statik değişken (local static) olarak tutulur.
     * * @return std::unordered_map<size_t, ComponentUIInfo>& Dinamik harita referansı.
     */
    static std::unordered_map<size_t, ComponentUIInfo>& get_components() {
        static std::unordered_map<size_t, ComponentUIInfo> components;
        return components;
    }
};