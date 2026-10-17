/**
 * @file scene.cpp
 * @brief Sahne yaşam döngüsü, varlık yönetimi, script çağrıları ve ışık çıkarma uygulaması.
 */

#include "scene.h"
#include "../Systems/light_system.h"
#include "../Scene/scriptable_entity.h"
#include "../Project/project_module.h"
#include "../Engine/Scripting/script_factory.h"
#include <iostream>
#include <vector>

struct EngineContext;

entity scene::create_entity()
{
    entity e = reg.create_entity();
    entities.push_back(e);
    return e;
}

void scene::new_scene()
{
    reg = registry();
    entities.clear();
    activeCamera = 0;
    lights.clear();
}

void scene::on_runtime_start(EngineContext& ctx)
{
    project::project_module::instance().sync_context();
    registry& currentReg = get_registry();

    // 1. Modül sonradan yüklendiği veya sahne modülden önce açıldığı için bağlanmamış betikleri fabrikadan bağla
    std::vector<std::pair<entity, std::string>> scriptsToBind;
    for (const auto& [e, scriptComp] : currentReg.get_all<native_script_component>())
    {
        if (!scriptComp.bind_instantiate && !scriptComp.script_name.empty() && scriptComp.script_name != "None")
        {
            scriptsToBind.emplace_back(e, scriptComp.script_name);
        }
    }
    for (const auto& [e, name] : scriptsToBind)
    {
        script_factory::instantiate_script(name, e, currentReg);
    }

    // 2. Betik örneklerini oluştur ve on_create olaylarını tetikle
    for (auto& [e, scriptComp] : currentReg.get_all<native_script_component>())
    {
        if (!scriptComp.bind_instantiate) {
            std::cerr << "[Scene] Uyari: '" << scriptComp.script_name << "' icin bind_instantiate bulunamadi! (Entity ID: " << e << ")\n";
            continue;
        }

        scriptComp.instance = scriptComp.bind_instantiate();
        if (!scriptComp.instance) {
            std::cerr << "[Scene] Hata: '" << scriptComp.script_name << "' instance olusturulamadi! (Entity ID: " << e << ")\n";
            continue;
        }
        scriptComp.instance->set_entity(e);
        scriptComp.instance->set_registry(&currentReg);
        scriptComp.instance->setContext(&ctx);
        scriptComp.instance->on_create();
        std::cout << "[Scene] Native script baslatildi: " << scriptComp.script_name << " (Entity ID: " << e << ")\n";
    }
}

void scene::on_update_runtime(float deltaTime)
{
    project::project_module::instance().sync_context();
    registry& currentReg = get_registry();
    for (auto& [e, scriptComp] : currentReg.get_all<native_script_component>())
    {
        if (!scriptComp.instance) {
            continue;
        }

        scriptComp.instance->on_update(deltaTime);
    }
}

void scene::on_fixed_update_runtime(float fixedDeltaTime)
{
    project::project_module::instance().sync_context();
    registry& currentReg = get_registry();
    for (auto& [e, scriptComp] : currentReg.get_all<native_script_component>())
    {
        if (!scriptComp.instance) {
            continue;
        }

        scriptComp.instance->on_fixed_update(fixedDeltaTime);
    }
}

void scene::clean_up_scripts()
{
    registry& currentReg = get_registry();
    for (auto& [e, scriptComp] : currentReg.get_all<native_script_component>())
    {
        if (scriptComp.instance)
        {
            // 1. Oyun mantığı yok etme olayını tetikle
            scriptComp.instance->on_destroy();

            // 2. Bellek serbest bırakma fonksiyonunu bileşen üzerinden çalıştır
            if (scriptComp.bind_destroy)
            {
                scriptComp.bind_destroy(&scriptComp);
            }
        }
    }
}

registry& scene::get_registry() noexcept
{
    return reg;
}

const registry& scene::get_registry() const noexcept
{
    return reg;
}

const std::vector<entity>& scene::get_entities() const noexcept
{
    return entities;
}

void scene::set_entities(const std::vector<entity>& newEntities)
{
    entities = newEntities;
}

void scene::set_active_camera(entity e) noexcept
{
    activeCamera = e;
}

entity scene::get_active_camera() const noexcept
{
    return activeCamera;
}

void scene::update_light_batch()
{
    lights.clear();
    lights.lights = light_system::extract(reg);
}