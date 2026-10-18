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
            typename std::enable_if<!std::is_same<T, glm::vec2>::value && !std::is_same<T, glm::vec3>::value && !std::is_same<T, glm::vec4>::value>::type* = nullptr)
        {
            j[key] = value;
        }

        static void write(json& j, const std::string& key, const glm::vec2& value)
        {
            j[key] = { value.x, value.y };
        }

        /**
         * @brief Matematiksel tipler (glm::vec3) için özelleştirilmiş yazma işlemi. Vektörü JSON dizisine dönüştürür.
         */
        static void write(json& j, const std::string& key, const glm::vec3& value)
        {
            j[key] = { value.x, value.y, value.z };
        }

        static void write(json& j, const std::string& key, const glm::vec4& value)
        {
            j[key] = { value.x, value.y, value.z, value.w };
        }

        /**
         * @brief Standart veri tiplerini JSON nesnesinden okur ve degiskene atar (Hata ve tip uyusmazligi korumali).
         */
        template <typename T>
        static void read(const json& j, const std::string& key, T& value,
            typename std::enable_if<!std::is_same<T, glm::vec2>::value && !std::is_same<T, glm::vec3>::value && !std::is_same<T, glm::vec4>::value>::type* = nullptr)
        {
            if (j.contains(key))
            {
                try
                {
                    value = j.at(key).get<T>();
                }
                catch (...)
                {
                    if constexpr (std::is_enum_v<T>)
                    {
                        try
                        {
                            if (j.at(key).is_number_integer())
                            {
                                value = static_cast<T>(j.at(key).get<int>());
                            }
                        }
                        catch (...) {}
                    }
                }
            }
        }

        static void read(const json& j, const std::string& key, glm::vec2& value)
        {
            if (j.contains(key))
            {
                try
                {
                    const auto& item = j.at(key);
                    if (item.is_array() && item.size() >= 2)
                    {
                        value.x = item[0].get<float>();
                        value.y = item[1].get<float>();
                    }
                    else if (item.is_object())
                    {
                        if (item.contains("x")) value.x = item["x"].get<float>();
                        if (item.contains("y")) value.y = item["y"].get<float>();
                    }
                }
                catch (...) {}
            }
        }

        /**
         * @brief JSON dizisinden veya nesnesinden ({x,y,z}) glm::vec3 degerini okur.
         */
        static void read(const json& j, const std::string& key, glm::vec3& value)
        {
            if (j.contains(key))
            {
                try
                {
                    const auto& item = j.at(key);
                    if (item.is_array() && item.size() >= 3)
                    {
                        value.x = item[0].get<float>();
                        value.y = item[1].get<float>();
                        value.z = item[2].get<float>();
                    }
                    else if (item.is_object())
                    {
                        if (item.contains("x")) value.x = item["x"].get<float>();
                        if (item.contains("y")) value.y = item["y"].get<float>();
                        if (item.contains("z")) value.z = item["z"].get<float>();
                    }
                }
                catch (...) {}
            }
        }

        static void read(const json& j, const std::string& key, glm::vec4& value)
        {
            if (j.contains(key))
            {
                try
                {
                    const auto& item = j.at(key);
                    if (item.is_array() && item.size() >= 4)
                    {
                        value.x = item[0].get<float>();
                        value.y = item[1].get<float>();
                        value.z = item[2].get<float>();
                        value.w = item[3].get<float>();
                    }
                    else if (item.is_object())
                    {
                        if (item.contains("x")) value.x = item["x"].get<float>();
                        else if (item.contains("r")) value.x = item["r"].get<float>();

                        if (item.contains("y")) value.y = item["y"].get<float>();
                        else if (item.contains("g")) value.y = item["g"].get<float>();

                        if (item.contains("z")) value.z = item["z"].get<float>();
                        else if (item.contains("b")) value.z = item["b"].get<float>();

                        if (item.contains("w")) value.w = item["w"].get<float>();
                        else if (item.contains("a")) value.w = item["a"].get<float>();
                    }
                }
                catch (...) {}
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