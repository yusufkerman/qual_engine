/**
 * @file script_factory.h
 * @brief Yeni scriptleri motora otomatik bildiren ve dinamik bağlayan fabrika sistemi.
 */

#pragma once
#include <string>
#include <unordered_map>
#include <functional>
#include "../ECS/registry.h"
#include "../ECS/components.h"

class script_factory {
public:
    // Bir script'i bağlamak için gereken fonksiyon kalıbı (Lambda imzası)
    using CreatorFunc = std::function<void(entity, registry&)>;

    /**
     * @brief Çalışma zamanında yeni bir script şablonunu fabrikaya kaydeder.
     */
    static void register_script(const std::string& name, CreatorFunc func) {
        get_registry()[name] = func;
    }

    /**
     * @brief Kayıtlı bir script şablonunu fabrikadan kaldırır.
     */
    static void unregister_script(const std::string& name) {
        get_registry().erase(name);
    }

    /**
     * @brief Fabrikadaki tüm script kayıtlarını temizler (Proje değiştiğinde kullanılır).
     */
    static void unregister_all_scripts() {
        get_registry().clear();
    }

    /**
     * @brief Diskten okunan isme göre ilgili script'i entity'ye otomatik bind eder.
     */
    static bool instantiate_script(const std::string& name, entity e, registry& reg) {
        auto& repo = get_registry();
        auto it = repo.find(name);
        if (it != repo.end()) {
            it->second(e, reg); // İlgili şablon lambdasını tetikle
            return true;
        }
        return false;
    }

    static std::vector<std::string> get_all_script_names() {
        std::vector<std::string> names;
        for (const auto& [name, _] : get_registry()) {
            names.push_back(name);
        }
        return names;
    }

private:
    // Static initialization krizlerini engellemek için yerel static map
    static std::unordered_map<std::string, CreatorFunc>& get_registry() {
        static std::unordered_map<std::string, CreatorFunc> instance;
        return instance;
    }
};

/**
 * @brief Sandbox tarafındaki scriptlerin tek satırda kendini otomatik kaydetmesini sağlayan yardımcı makro.
 */
#define REGISTER_SCRIPT(ScriptClass) \
    inline bool ScriptClass##_registered = ([]() { \
        script_factory::register_script(#ScriptClass, [](entity e, registry& reg) { \
            auto& nsc = reg.add_component<native_script_component>(e); \
            nsc.bind<ScriptClass>(#ScriptClass); \
        }); \
        return true; \
    })()