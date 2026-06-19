#include "registry.h"

// ENTITY
entity registry::create_entity() {
    entity e = next_entity++;
    entities.insert(e);
    return e;
}

void registry::add_entity(entity e)
{
    entities.insert(e);
}

const std::unordered_set<entity>& registry::get_all_entities() const {
    return entities;
}

// TRANSFORM
void registry::add_transform(entity e, const transform_component& t) {
    transforms[e] = t;
}

transform_component* registry::get_transform(entity e) {
    auto it = transforms.find(e);
    return it != transforms.end() ? &it->second : nullptr;
}

const transform_component* registry::get_transform(entity e) const {
    auto it = transforms.find(e);
    return it != transforms.end() ? &it->second : nullptr;
}

void registry::remove_transform(entity e) {
    transforms.erase(e);
}

bool registry::has_transform(entity e) const {
    return transforms.find(e) != transforms.end();
}

// MESH
void registry::add_mesh(entity e, const mesh_component& m) {
    meshes[e] = m;
}

mesh_component* registry::get_mesh(entity e) {
    auto it = meshes.find(e);
    return it != meshes.end() ? &it->second : nullptr;
}

const mesh_component* registry::get_mesh(entity e) const {
    auto it = meshes.find(e);
    return it != meshes.end() ? &it->second : nullptr;
}

void registry::remove_mesh(entity e) {
    meshes.erase(e);
}

bool registry::has_mesh(entity e) const {
    return meshes.find(e) != meshes.end();
}

// CAMERA
void registry::add_camera(entity e, const camera_component& c) {
    cameras[e] = c;
}

camera_component* registry::get_camera(entity e) {
    auto it = cameras.find(e);
    return it != cameras.end() ? &it->second : nullptr;
}

const camera_component* registry::get_camera(entity e) const {
    auto it = cameras.find(e);
    return it != cameras.end() ? &it->second : nullptr;
}

void registry::remove_camera(entity e) {
    cameras.erase(e);
}

bool registry::has_camera(entity e) const {
    return cameras.find(e) != cameras.end();
}

// LIGHT
void registry::add_light(entity e, const light_component& l) {
    lights[e] = l;
}

light_component* registry::get_light(entity e) {
    auto it = lights.find(e);
    return it != lights.end() ? &it->second : nullptr;
}

const light_component* registry::get_light(entity e) const {
    auto it = lights.find(e);
    return it != lights.end() ? &it->second : nullptr;
}

void registry::remove_light(entity e) {
    lights.erase(e);
}

bool registry::has_light(entity e) const {
    return lights.find(e) != lights.end();
}

const std::unordered_map<entity, mesh_component>& registry::get_meshes() const {
    return meshes;
}

const std::unordered_map<entity, light_component>& registry::get_lights() const {
    return lights;
}