#pragma once

#include <unordered_map>
#include <unordered_set>

#include "entity.h"
#include "components.h"

class registry {
public:
    entity create_entity();
    void add_entity(entity e);

    void add_transform(entity e, const transform_component& t);
    void add_mesh(entity e, const mesh_component& m);
    void add_camera(entity e, const camera_component& c);
    void add_light(entity e, const light_component& l);

    void remove_transform(entity e);
    void remove_mesh(entity e);
    void remove_camera(entity e);
    void remove_light(entity e);

    transform_component* get_transform(entity e);
    mesh_component* get_mesh(entity e);
    camera_component* get_camera(entity e);
    light_component* get_light(entity e);

    const transform_component* get_transform(entity e) const;
    const mesh_component* get_mesh(entity e) const;
    const camera_component* get_camera(entity e) const;
    const light_component* get_light(entity e) const;

    bool has_transform(entity e) const;
    bool has_mesh(entity e) const;
    bool has_camera(entity e) const;
    bool has_light(entity e) const;

    const std::unordered_set<entity>& get_all_entities() const;

    const std::unordered_map<entity, mesh_component>& get_meshes() const;
    const std::unordered_map<entity, light_component>& get_lights() const;

private:
    entity next_entity = 1;

    std::unordered_set<entity> entities;

    std::unordered_map<entity, transform_component> transforms;
    std::unordered_map<entity, mesh_component> meshes;
    std::unordered_map<entity, camera_component> cameras;
    std::unordered_map<entity, light_component> lights;
};