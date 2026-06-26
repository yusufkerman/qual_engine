#pragma once

#include <string>
#include <vector>
#include "../ECS/registry.h"
#include "../Systems/camera_system.h"
#include "../Renderer/light_batch.h"

struct EngineContext;


class scene
{
public:
    scene() = default;

    entity create_entity();
    void new_scene();

    registry& get_registry();
    const registry& get_registry() const;

    const std::vector<entity>& get_entities() const;
    void set_entities(const std::vector<entity>& entities);

    void set_active_camera(entity newCam);
    entity get_active_camera() const;

    void update_light_batch();

    void on_runtime_start(EngineContext& ctx);
	void on_update_runtime(float deltaTime);
    void clean_up_scripts();

    light_batch& get_light_batch()
    {
        return lights;
    }

    const light_batch& get_light_batch() const
    {
        return lights;
    }

private:
    registry reg;
    std::vector<entity> entities;

    entity activeCamera = 0;
    light_batch lights;
};