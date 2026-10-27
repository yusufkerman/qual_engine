/**
 * @file scene.cpp
 * @brief Sahne yasam dongusu, varlik yonetimi, script cagrilarinin guvenli calistirilmasi ve isik cikarma uygulamasi.
 */

#include "scene.h"
#include "../Systems/light_system.h"
#include "../Scene/scriptable_entity.h"
#include "../Project/project_module.h"
#include "../Engine/Scripting/script_factory.h"
#include "../Scripting/script_invoker.h"
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

    // 1. Modul sonradan yuklendigi veya sahne modÃ¼lden once acildigi icin baglanmamis betikleri fabrikadan bagla
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

    // 2. Betik orneklerini olustur ve on_create olaylarini guvenli calistir
    for (auto& [e, scriptComp] : currentReg.get_all<native_script_component>())
    {
        if (!scriptComp.is_enabled || scriptComp.is_quarantined) {
            continue;
        }

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

        // SEH Kalkanı ile korumalı on_create çağrısı
        bool ok = qual::scripting::script_invoker::invoke(
            scriptComp.script_name,
            static_cast<uint32_t>(e),
            "on_create",
            [&]() { scriptComp.instance->on_create(); },
            &scriptComp.last_error
        );

        if (!ok) {
            scriptComp.is_quarantined = true;
            scriptComp.is_enabled = false;
            if (scriptComp.bind_destroy) {
                scriptComp.bind_destroy(&scriptComp);
            }
            scriptComp.instance = nullptr;
            std::cerr << "[Scene] Betik on_create istisnasi nedeniyle karantinaya alindi: " 
                      << scriptComp.script_name << " (Entity ID: " << e << ")\n";
            continue;
        }

        std::cout << "[Scene] Native script baslatildi: " << scriptComp.script_name << " (Entity ID: " << e << ")\n";
    }
}

void scene::on_update_runtime(float deltaTime)
{
    project::project_module::instance().sync_context();
    registry& currentReg = get_registry();
    for (auto& [e, scriptComp] : currentReg.get_all<native_script_component>())
    {
        if (!scriptComp.instance || !scriptComp.is_enabled || scriptComp.is_quarantined) {
            continue;
        }

        bool ok = qual::scripting::script_invoker::invoke(
            scriptComp.script_name,
            static_cast<uint32_t>(e),
            "on_update",
            [&]() { scriptComp.instance->on_update(deltaTime); },
            &scriptComp.last_error
        );

        if (!ok) {
            scriptComp.is_quarantined = true;
            scriptComp.is_enabled = false;
            // Bozulmuş nesne üzerinde on_destroy çağrılmadan bellek temizliği yapılır
            if (scriptComp.bind_destroy) {
                scriptComp.bind_destroy(&scriptComp);
            }
            scriptComp.instance = nullptr;
            std::cerr << "[Scene] Betik on_update istisnasi nedeniyle karantinaya alindi: " 
                      << scriptComp.script_name << " (Entity ID: " << e << ")\n";
        }
    }
}

void scene::on_fixed_update_runtime(float fixedDeltaTime)
{
    project::project_module::instance().sync_context();
    registry& currentReg = get_registry();
    for (auto& [e, scriptComp] : currentReg.get_all<native_script_component>())
    {
        if (!scriptComp.instance || !scriptComp.is_enabled || scriptComp.is_quarantined) {
            continue;
        }

        bool ok = qual::scripting::script_invoker::invoke(
            scriptComp.script_name,
            static_cast<uint32_t>(e),
            "on_fixed_update",
            [&]() { scriptComp.instance->on_fixed_update(fixedDeltaTime); },
            &scriptComp.last_error
        );

        if (!ok) {
            scriptComp.is_quarantined = true;
            scriptComp.is_enabled = false;
            if (scriptComp.bind_destroy) {
                scriptComp.bind_destroy(&scriptComp);
            }
            scriptComp.instance = nullptr;
            std::cerr << "[Scene] Betik on_fixed_update istisnasi nedeniyle karantinaya alindi: " 
                      << scriptComp.script_name << " (Entity ID: " << e << ")\n";
        }
    }
}

void scene::clean_up_scripts()
{
    registry& currentReg = get_registry();
    for (auto& [e, scriptComp] : currentReg.get_all<native_script_component>())
    {
        if (scriptComp.instance)
        {
            // Eğer betik zaten karantinaya alınmışsa on_destroy çağrılmaz (ikinci istisna önlenir)
            if (!scriptComp.is_quarantined) {
                qual::scripting::script_invoker::invoke(
                    scriptComp.script_name,
                    static_cast<uint32_t>(e),
                    "on_destroy",
                    [&]() { scriptComp.instance->on_destroy(); },
                    &scriptComp.last_error
                );
            }

            // Bellek serbest bırakma fonksiyonunu bileşen üzerinden çalıştır
            if (scriptComp.bind_destroy)
            {
                scriptComp.bind_destroy(&scriptComp);
            }
            scriptComp.instance = nullptr;
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
