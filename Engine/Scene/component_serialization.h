#pragma once

/**
 * @file scene_serializer.h
 * @brief ECS bileşenleri, GLM vektörleri ve Aria varlık tanıtıcıları için JSON serileştirme tanımları.
 */

#include <tuple>
#include <string>
#include <glm/glm.hpp>
#include "../Externals/nlohmann/json.hpp"
#include "../ECS/entity.h"
#include "../ECS/registry.h"
#include "../ECS/components.h"
#include "../Renderer/material.h"
#include "../Engine/Assets/aria.h"

using json = nlohmann::json;

// ======================================================
// 1. ARIA HANDLE SERIALIZATION (NLOHMANN ADL SERIALIZER)
// ======================================================
namespace nlohmann {

    /**
     * @brief Aria varlık tanıtıcıları (handle) için JSON serileştirme uzmanlaşımı (ADL serializer).
     */
    template <typename T>
    struct adl_serializer<aria::handle<T>>
    {
        static void to_json(json& j, const aria::handle<T>& h)
        {
            j = json{ {"index", h.index}, {"generation", h.generation} };
        }

        static void from_json(const json& j, aria::handle<T>& h)
        {
            h.index = j.value("index", 0u);
            h.generation = j.value("generation", 0u);
        }
    };

} // namespace nlohmann

// ======================================================
// 2. GLM SERIALIZATION
// ======================================================
namespace glm {

    inline void to_json(json& j, const vec3& v)
    {
        j = json{ {"x", v.x}, {"y", v.y}, {"z", v.z} };
    }

    inline void from_json(const json& j, vec3& v)
    {
        try {
            if (j.is_array() && j.size() >= 3)
            {
                v.x = j[0].get<float>();
                v.y = j[1].get<float>();
                v.z = j[2].get<float>();
            }
            else if (j.is_object())
            {
                v.x = j.value("x", 0.0f);
                v.y = j.value("y", 0.0f);
                v.z = j.value("z", 0.0f);
            }
        } catch (...) {}
    }

    inline void to_json(json& j, const vec2& v)
    {
        j = json{ {"x", v.x}, {"y", v.y} };
    }

    inline void from_json(const json& j, vec2& v)
    {
        try {
            if (j.is_array() && j.size() >= 2)
            {
                v.x = j[0].get<float>();
                v.y = j[1].get<float>();
            }
            else if (j.is_object())
            {
                v.x = j.value("x", 0.0f);
                v.y = j.value("y", 0.0f);
            }
        } catch (...) {}
    }

    inline void to_json(json& j, const vec4& v)
    {
        j = json{ {"x", v.x}, {"y", v.y}, {"z", v.z}, {"w", v.w} };
    }

    inline void from_json(const json& j, vec4& v)
    {
        try {
            if (j.is_array() && j.size() >= 4)
            {
                v.x = j[0].get<float>();
                v.y = j[1].get<float>();
                v.z = j[2].get<float>();
                v.w = j[3].get<float>();
            }
            else if (j.is_object())
            {
                v.x = j.value("x", j.value("r", 0.0f));
                v.y = j.value("y", j.value("g", 0.0f));
                v.z = j.value("z", j.value("b", 0.0f));
                v.w = j.value("w", j.value("a", 0.0f));
            }
        } catch (...) {}
    }

} // namespace glm

// ======================================================
// 3. ENUM SERIALIZATION
// ======================================================
NLOHMANN_JSON_SERIALIZE_ENUM(light_type, {
    {light_type::directional, "directional"},
    {light_type::point, "point"},
    {light_type::spot, "spot"}
    })

    NLOHMANN_JSON_SERIALIZE_ENUM(motion_type, {
        {motion_type::static_body, "static"},
        {motion_type::dynamic_body, "dynamic"},
        {motion_type::kinematic_body, "kinematic"}
        })

    NLOHMANN_JSON_SERIALIZE_ENUM(collider_type, {
        {collider_type::box, "box"},
        {collider_type::sphere, "sphere"}
        })

    NLOHMANN_JSON_SERIALIZE_ENUM(mesh_source_type, {
        {mesh_source_type::model, "model"},
        {mesh_source_type::primitive_cube, "cube"},
        {mesh_source_type::primitive_sphere, "sphere"}
        })

    NLOHMANN_JSON_SERIALIZE_ENUM(canvas_render_mode, {
        {canvas_render_mode::screen_space_overlay, "screen_space_overlay"},
        {canvas_render_mode::world_space, "world_space"}
        })

    NLOHMANN_JSON_SERIALIZE_ENUM(canvas_scaler_mode, {
        {canvas_scaler_mode::constant_pixel_size, "constant_pixel_size"},
        {canvas_scaler_mode::scale_with_screen_size, "scale_with_screen_size"}
        })

    NLOHMANN_JSON_SERIALIZE_ENUM(ui_text_alignment, {
        {ui_text_alignment::top_left, "top_left"},
        {ui_text_alignment::top_center, "top_center"},
        {ui_text_alignment::top_right, "top_right"},
        {ui_text_alignment::middle_left, "middle_left"},
        {ui_text_alignment::center, "center"},
        {ui_text_alignment::middle_right, "middle_right"},
        {ui_text_alignment::bottom_left, "bottom_left"},
        {ui_text_alignment::bottom_center, "bottom_center"},
        {ui_text_alignment::bottom_right, "bottom_right"}
        })

    NLOHMANN_JSON_SERIALIZE_ENUM(ui_button_state, {
        {ui_button_state::normal, "normal"},
        {ui_button_state::hovered, "hovered"},
        {ui_button_state::pressed, "pressed"},
        {ui_button_state::disabled, "disabled"}
        })

    // ======================================================
    // 4. COMPONENT SERIALIZATION
    // ======================================================
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(material,
        shader_id, base_color,
        diffuse_texture, normal_texture,
        specular_texture, normal_strength,
        texture_scale,
        specular_strength, shininess)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(name_component, name)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(transform_component,
        position, rotation, scale, boundingRadius)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(mesh_component,
        model_handle, sub_mesh_index)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(material_component, material_handle)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(camera_component,
        fov, nearPlane, farPlane, primary, priority)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(light_component,
        type, color, intensity, range,
        inner_cone, outer_cone, cast_shadows)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(bounds_component,
        localCenter, localRadius)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(rigidbody_component,
        motion, mass, friction, restitution)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(collider_component,
        type, extents)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(skinned_mesh_component,
        model_handle, sub_mesh_index, cast_shadows)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(skeleton_component,
        skeleton_handle)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(animator_component,
        current_clip, playback_time, playback_speed, is_playing, is_looping, apply_root_motion)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(bone_attachment_component,
        target_skeleton_entity, bone_name, local_offset, local_rotation, local_scale)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(canvas_component,
        render_mode, scaler_mode, reference_resolution, match_width_or_height, sorting_order)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(rect_transform_component,
        anchored_position, size, anchor_min, anchor_max, pivot, rotation, scale, visible)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ui_text_component,
        text, font_path, font_size, color, alignment, wrap_text, line_spacing)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ui_image_component,
        color, raycast_target)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ui_button_component,
        interactable, normal_color, hover_color, pressed_color, disabled_color)

    // ======================================================
    // 5. NATIVE SCRIPT COMPONENT
    // ======================================================
    inline void to_json(json& j, const native_script_component& nsc)
{
    j = json{ {"script_name", nsc.script_name} };
}

inline void from_json(const json& j, native_script_component& nsc)
{
    nsc.script_name = j.value("script_name", "None");
}

// ======================================================
// 6. ENGINE COMPONENT TUPLE REGISTRY
// ======================================================
using EngineComponentList = std::tuple<
    name_component,
    transform_component,
    mesh_component,
    material_component,
    camera_component,
    light_component,
    bounds_component,
    rigidbody_component,
    collider_component,
    skinned_mesh_component,
    skeleton_component,
    animator_component,
    bone_attachment_component,
    canvas_component,
    rect_transform_component,
    ui_text_component,
    ui_image_component,
    ui_button_component
>;

// ======================================================
// 7. GENERIC COMPONENT SERIALIZATION HELPERS
// ======================================================

/**
 * @brief Varlıkta belirtilen bileşen mevcutsa JSON nesnesine yazar.
 */
template <typename T>
void serialize_component_if_exists(entity e, const registry& reg, json& entityJson, const std::string& key)
{
    if (const T* comp = reg.get_component<T>(e))
    {
        entityJson[key] = *comp;
    }
}

/**
 * @brief JSON nesnesinde anahtar mevcutsa bileşeni çözüp varlığa ekler.
 */
template <typename T>
void deserialize_component_if_exists(entity e, registry& reg, const json& entityJson, const std::string& key)
{
    if (!entityJson.contains(key))
    {
        return;
    }

    T comp = entityJson.at(key).get<T>();
    reg.add_component<T>(e, comp);
}