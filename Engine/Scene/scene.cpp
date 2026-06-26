#include "scene.h"
#include "../Systems/light_system.h"
#include "../Scene/scriptable_entity.h"

#include <iostream>

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
    registry& reg = get_registry();
    for (auto& [e, scriptComp] : reg.get_all<native_script_component>())
    {
        if (!scriptComp.bind_instantiate) continue;
        scriptComp.instance = scriptComp.bind_instantiate();
        scriptComp.instance->set_entity(e);
        scriptComp.instance->set_registry(&reg);
        scriptComp.instance->setContext(&ctx);
        scriptComp.instance->on_create();
    }
}
void scene::on_update_runtime(float deltaTime)
{
	registry& reg = get_registry();
    for (auto [e, scriptComp] : reg.get_all<native_script_component>()) 
    {
        if (!scriptComp.instance) continue;

        scriptComp.instance->set_entity(e);
		scriptComp.instance->set_registry(&reg);

		scriptComp.instance->on_update(deltaTime);
    }
}
void scene::clean_up_scripts()
{
    registry& reg = get_registry();
    for (auto [e, scriptComp] : reg.get_all<native_script_component>())
    {
        if (scriptComp.instance)
        {
            // 1. Önce oyun mantığı yok etme fonksiyonunu çağırıyoruz
            scriptComp.instance->on_destroy();

            // 2. Ardından bellek silme işini komponentin tuttuğu fonksiyona yaptırıyoruz
            scriptComp.bind_destroy(&scriptComp);
        }
    }
}
registry& scene::get_registry()
{
    return reg;
}

const registry& scene::get_registry() const
{
    return reg;
}

const std::vector<entity>& scene::get_entities() const
{
    return entities;
}

void scene::set_entities(const std::vector<entity>& newEntities)
{
    entities = newEntities;
}

void scene::set_active_camera(entity e)
{
    activeCamera = e;
}

entity scene::get_active_camera() const
{
    return activeCamera;
}

void scene::update_light_batch()
{
    lights.clear();
    lights.lights = light_system::extract(reg);
}