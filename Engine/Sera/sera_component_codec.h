#pragma once

/**
 * @file sera_component_codec.h
 * @brief Makro tabanlı derleme zamanı (compile-time) yansıma (reflection) ve bileşen serileştirme mekanizması.
 */

#include <string>
#include <type_traits>
#include "../Externals/nlohmann/json.hpp"
#include <glm/glm.hpp>

using json = nlohmann::json;

namespace sera {

    // ============================================================================
    // 1. SERIALIZER DISPATCH (TİP YÖNLENDİRME)
    // ============================================================================

    /**
     * @struct serializer_dispatch
     * @brief Tip özelliklerine (type traits) göre JSON okuma ve yazma işlemlerini yönlendiren statik veri yapısı.
     */
    struct serializer_dispatch
    {
        /**
         * @brief Standart veri tiplerini (int, float, bool, std::string) doğrudan JSON nesnesine yazar.
         */
        template <typename T>
        static void write(json& j, const std::string& key, const T& value,
            typename std::enable_if<!std::is_same<T, glm::vec3>::value>::type* = nullptr)
        {
            j[key] = value;
        }

        /**
         * @brief Matematiksel tipler (glm::vec3) için özelleştirilmiş yazma işlemi. Vektörü JSON dizisine dönüştürür.
         */
        static void write(json& j, const std::string& key, const glm::vec3& value)
        {
            j[key] = { value.x, value.y, value.z };
        }

        /**
         * @brief Standart veri tiplerini JSON nesnesinden okur ve değişkene atar.
         */
        template <typename T>
        static void read(const json& j, const std::string& key, T& value,
            typename std::enable_if<!std::is_same<T, glm::vec3>::value>::type* = nullptr)
        {
            if (j.contains(key))
            {
                value = j.at(key).get<T>();
            }
        }

        /**
         * @brief JSON dizisinden glm::vec3 değerini okur ve vektör bileşenlerine ayrıştırır.
         */
        static void read(const json& j, const std::string& key, glm::vec3& value)
        {
            if (j.contains(key) && j.at(key).is_array() && j.at(key).size() == 3)
            {
                value.x = j.at(key)[0].get<float>();
                value.y = j.at(key)[1].get<float>();
                value.z = j.at(key)[2].get<float>();
            }
        }
    };

    // ============================================================================
    // 2. MAKRO TABANLI YANSIMA (REFLECTION) ARAÇLARI
    // ============================================================================

    /**
     * @def SERA_FIELD(field_name)
     * @brief Sınıf/yapı alanını ismi ve kendisi ile paketler.
     * @note Makro kapsamındaki "component_obj" referansını kullanarak güvenli erişim sağlar.
     */
#define SERA_FIELD(field_name) #field_name, component_obj.field_name

     /**
      * @def DESCRIBE_COMPONENT(comp_type, ...)
      * @brief Belirtilen bileşen türü için derleme zamanı serileştirme arayüzünü (factory) oluşturur.
      * @note Hem const (okuma/kaydetme) hem non-const (yazma/yükleme) aşırı yüklemelerini üretir.
      */
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
      // 3. JENERİK İŞLEYİCİLER (SERIALIZE / DESERIALIZE MOTORU)
      // ============================================================================

      /**
       * @struct serialize_visitor
       * @brief Yansıtılan alanları dolaşarak (variadic template) JSON formatına çeviren ziyaretçi sınıf.
       */
    struct serialize_visitor
    {
        json& out_json; ///< Verilerin yazılacağı hedef JSON referansı.

        /**
         * @brief Gelen tüm parametre paketini (args) özyinelemeli döngüye iletir.
         */
        template<typename... Args>
        void operator()(Args&&... args)
        {
            process_fields(std::forward<Args>(args)...);
        }

    private:
        /**
         * @brief Özyinelemenin (recursion) bitiş koşulu.
         */
        void process_fields() {}

        /**
         * @brief Parametre paketindeki ilk veriyi işler ve kalanı bir sonraki çağrıya devreder.
         */
        template<typename T, typename... Args>
        void process_fields(const char* key, const T& value, Args&&... args)
        {
            serializer_dispatch::write(out_json, key, value);
            process_fields(std::forward<Args>(args)...);
        }
    };

    /**
     * @struct deserialize_visitor
     * @brief JSON verilerini dolaşarak yansıtılan bileşen alanlarını dolduran ziyaretçi sınıf.
     */
    struct deserialize_visitor
    {
        const json& in_json; ///< Verilerin okunacağı kaynak JSON referansı.

        /**
         * @brief Gelen tüm parametre paketini (args) özyinelemeli okuma döngüsüne iletir.
         */
        template<typename... Args>
        void operator()(Args&&... args)
        {
            process_fields(std::forward<Args>(args)...);
        }

    private:
        /**
         * @brief Özyinelemenin (recursion) bitiş koşulu.
         */
        void process_fields() {}

        /**
         * @brief Parametre paketindeki ilk veriyi JSON'dan okur ve kalanı bir sonraki çağrıya devreder.
         */
        template<typename T, typename... Args>
        void process_fields(const char* key, T& value, Args&&... args)
        {
            serializer_dispatch::read(in_json, key, value);
            process_fields(std::forward<Args>(args)...);
        }
    };

} // namespace sera