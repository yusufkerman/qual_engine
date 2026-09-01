/**
 * @file sera_component_codec.h
 * @brief Makro tabanlı Compile-Time Reflection ve Bileşen Serileştirme Mekanizması.
 */
#pragma once

#include <string>
#include <type_traits>
#include "../Externals/nlohmann/json.hpp"
#include <glm/glm.hpp>

using json = nlohmann::json;

namespace sera {

    // ============================================================================
    // 1. (SERIALIZER DISPATCH): Tiplere göre otomatik yönlendirme
    // ============================================================================
    struct serializer_dispatch {

        // A. Düz Tipler: int, float, bool, std::string vb. direkt JSON'a yazılır.
        template <typename T>
        static void write(json& j, const std::string& key, const T& value,
            typename std::enable_if<!std::is_same<T, glm::vec3>::value>::type* = nullptr) {
            j[key] = value;
        }

        // B. Matematiksel Tipler: glm::vec3 yakalandığı an otomatik float dizisine çevrilir.
        static void write(json& j, const std::string& key, const glm::vec3& value) {
            j[key] = { value.x, value.y, value.z };
        }

        // C. Düz Tipler Okuma
        template <typename T>
        static void read(const json& j, const std::string& key, T& value,
            typename std::enable_if<!std::is_same<T, glm::vec3>::value>::type* = nullptr) {
            if (j.contains(key)) {
                value = j.at(key).get<T>();
            }
        }

        // D. Matematiksel Tipler Okuma
        static void read(const json& j, const std::string& key, glm::vec3& value) {
            if (j.contains(key) && j.at(key).is_array() && j.at(key).size() == 3) {
                value.x = j.at(key)[0].get<float>();
                value.y = j.at(key)[1].get<float>();
                value.z = j.at(key)[2].get<float>();
            }
        }
    };

    // ============================================================================
    // 2. Sınıf/Struct içini amelelik yapmadan dışarıya haritalayan kısım
    // ============================================================================

    // Her bir değişken alanı (field) için ismi ve kendisini paketleyen yardımcı makro
    // Başına component_obj ekleyerek makro kapsamındaki (scope) nesneye güvenli erişim sağlıyoruz.
#define SERA_FIELD(field_name) #field_name, component_obj.field_name

// Hem const (Save için) hem non-const (Load için) referans aşırı yüklemelerini (overload)
// inline şablon fonksiyonlar olarak üreten, derleyicinin "tanımsız" demesini engelleyen nihai makro.
#define DESCRIBE_COMPONENT(comp_type, ...) \
        template<typename F> \
        inline void sera_reflect_component(comp_type& component_obj, F&& factory_fn) { \
            factory_fn(__VA_ARGS__); \
        } \
        template<typename F> \
        inline void sera_reflect_component(const comp_type& component_obj, F&& factory_fn) { \
            factory_fn(__VA_ARGS__); \
        }

    // ============================================================================
    // 3. GENERİC İŞLEYİCİLER (SERIALIZE / DESERIALIZE MOTORU)
    // ============================================================================

    // JSON'a yazma motoru: Makrodan gelen değişkenleri tek tek döngüye sokar
    struct serialize_visitor {
        json& out_json;

        // C++11 Variadic Parameter Pack kullanarak gelen tüm FIELD'ları sırayla işler
        template<typename... Args>
        void operator()(Args&&... args) {
            process_fields(std::forward<Args>(args)...);
        }

    private:
        void process_fields() {} // Döngü bitişi için boş gövde

        template<typename T, typename... Args>
        void process_fields(const char* key, const T& value, Args&&... args) {
            // Trafik polisine gönder, o tipine göre (vec3 mü, float mu) doğru yazacaktır
            serializer_dispatch::write(out_json, key, value);
            process_fields(std::forward<Args>(args)...); // Sıradaki değişkene geç (Özyineleme)
        }
    };

    // JSON'dan okuma motoru
    struct deserialize_visitor {
        const json& in_json;

        template<typename... Args>
        void operator()(Args&&... args) {
            process_fields(std::forward<Args>(args)...);
        }

    private:
        void process_fields() {}

        template<typename T, typename... Args>
        void process_fields(const char* key, T& value, Args&&... args) {
            serializer_dispatch::read(in_json, key, value);
            process_fields(std::forward<Args>(args)...);
        }
    };

} // namespace sera