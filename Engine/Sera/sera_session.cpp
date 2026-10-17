/**
 * @file sera_session.cpp
 * @brief SERA sahne kaydetme, yükleme, bileşen yansıma döngüleri ve dosya I/O uygulaması.
 */

#include "sera_session.h"
#include "sera_component_codec.h"
#include "../Engine/engine_context.h"
#include "../ECS/components.h"       
#include "../Engine/Assets/aria.h"
#include <fstream>
#include <iostream>
#include "../Engine/Scripting/script_factory.h"

namespace sera {

    sera_session::sera_session(EngineContext& ctx)
        : m_ctx(ctx), m_asset_resolver(ctx)
    {
    }

    validation_report sera_session::save(const std::string& filepath, const scene& current_scene)
    {
        validation_report report;
        const registry& reg = current_scene.get_registry();
        const auto& all_entities = reg.get_all_entities();

        json scene_json;
        scene_json["scene_name"] = "Sera_Engine_Scene";

        const entity active_cam = current_scene.get_active_camera();
        if (active_cam != 0)
        {
            scene_json["active_camera_uuid"] = m_entity_registry.get_uuid(active_cam);
        }
        else
        {
            scene_json["active_camera_uuid"] = "";
        }

        // Çevre, Sis ve Renk Ayarları (Environment)
        json env_json = json::object();
        env_json["fog"]["enabled"] = m_ctx.fog.enabled;
        env_json["fog"]["show_in_scene"] = m_ctx.fog.show_in_scene;
        env_json["fog"]["color"] = { m_ctx.fog.color.r, m_ctx.fog.color.g, m_ctx.fog.color.b };
        env_json["fog"]["density"] = m_ctx.fog.density;
        env_json["rendering"]["clear_color_scene"] = { m_ctx.renderSettings.clear_color_scene.r, m_ctx.renderSettings.clear_color_scene.g, m_ctx.renderSettings.clear_color_scene.b };
        env_json["rendering"]["clear_color_game"] = { m_ctx.renderSettings.clear_color_game.r, m_ctx.renderSettings.clear_color_game.g, m_ctx.renderSettings.clear_color_game.b };
        scene_json["environment"] = env_json;

        scene_json["entities"] = json::array();

        for (const entity e : all_entities)
        {
            json entity_json;

            std::string ent_uuid = m_entity_registry.get_uuid(e);
            if (ent_uuid.empty())
            {
                ent_uuid = sera_entity_registry::generate_uuid();
                m_entity_registry.register_link(e, ent_uuid);
            }
            entity_json["uuid"] = ent_uuid;

            entity_json["components"] = json::object();
            json& comps_out = entity_json["components"];

            // 1. Name Component
            if (reg.has_component<name_component>(e))
            {
                auto* comp = const_cast<registry&>(reg).get_component<name_component>(e);
                if (comp)
                {
                    json comp_json = json::object();
                    serialize_visitor visitor{ comp_json };
                    sera_reflect_component(*comp, visitor);
                    comps_out["name_component"] = comp_json;
                }
            }

            // 2. Transform Component
            if (reg.has_component<transform_component>(e))
            {
                auto* comp = const_cast<registry&>(reg).get_component<transform_component>(e);
                if (comp)
                {
                    json comp_json = json::object();
                    serialize_visitor visitor{ comp_json };
                    sera_reflect_component(*comp, visitor);
                    comps_out["transform_component"] = comp_json;
                }
            }

            // 3. Mesh Component (Aria Entegrasyonu)
            if (reg.has_component<mesh_component>(e))
            {
                auto* comp = const_cast<registry&>(reg).get_component<mesh_component>(e);
                if (comp)
                {
                    json comp_json = json::object();

                    const aria::asset_guid guid =
                        m_asset_resolver.to_guid<aria::model_asset>(comp->model_handle);

                    comp_json["model_guid"] = uuids::to_string(guid);
                    comp_json["sub_mesh_index"] = comp->sub_mesh_index;
                    comp_json["source"] = static_cast<int>(comp->source);

                    comps_out["mesh_component"] = comp_json;
                }
            }

            // 4. Material Component (Aria Entegrasyonu)
            if (reg.has_component<material_component>(e))
            {
                auto* comp = const_cast<registry&>(reg).get_component<material_component>(e);
                if (comp)
                {
                    json comp_json = json::object();

                    const aria::asset_guid guid =
                        m_asset_resolver.to_guid<aria::material_asset>(comp->material_handle);

                    comp_json["material_guid"] = uuids::to_string(guid);

                    comps_out["material_component"] = comp_json;
                }
            }

            // 5. Camera Component
            if (reg.has_component<camera_component>(e))
            {
                auto* comp = const_cast<registry&>(reg).get_component<camera_component>(e);
                if (comp)
                {
                    json comp_json = json::object();
                    serialize_visitor visitor{ comp_json };
                    sera_reflect_component(*comp, visitor);
                    comps_out["camera_component"] = comp_json;
                }
            }

            // 6. Light Component
            if (reg.has_component<light_component>(e))
            {
                auto* comp = const_cast<registry&>(reg).get_component<light_component>(e);
                if (comp)
                {
                    json comp_json = json::object();
                    serialize_visitor visitor{ comp_json };
                    sera_reflect_component(*comp, visitor);
                    comps_out["light_component"] = comp_json;
                }
            }

            // 7. Bounds Component
            if (reg.has_component<bounds_component>(e))
            {
                auto* comp = const_cast<registry&>(reg).get_component<bounds_component>(e);
                if (comp)
                {
                    json comp_json = json::object();
                    serialize_visitor visitor{ comp_json };
                    sera_reflect_component(*comp, visitor);
                    comps_out["bounds_component"] = comp_json;
                }
            }

            // 8. Rigidbody Component (Fizik parametreleri)
            if (reg.has_component<rigidbody_component>(e))
            {
                auto* comp = const_cast<registry&>(reg).get_component<rigidbody_component>(e);
                if (comp)
                {
                    json comp_json = json::object();
                    serialize_visitor visitor{ comp_json };
                    sera_reflect_component(*comp, visitor);
                    comps_out["rigidbody_component"] = comp_json;
                }
            }

            // 9. Collider Component
            if (reg.has_component<collider_component>(e))
            {
                auto* comp = const_cast<registry&>(reg).get_component<collider_component>(e);
                if (comp)
                {
                    json comp_json = json::object();
                    serialize_visitor visitor{ comp_json };
                    sera_reflect_component(*comp, visitor);
                    comps_out["collider_component"] = comp_json;
                }
            }

            // 10. Native Script Component
            if (reg.has_component<native_script_component>(e))
            {
                auto* comp = const_cast<registry&>(reg).get_component<native_script_component>(e);
                if (comp)
                {
                    json comp_json = json::object();
                    serialize_visitor visitor{ comp_json };
                    sera_reflect_component(*comp, visitor);
                    comps_out["native_script_component"] = comp_json;
                }
            }

            // 11. Skinned Mesh Component (Aria Entegrasyonu)
            if (reg.has_component<skinned_mesh_component>(e))
            {
                auto* comp = const_cast<registry&>(reg).get_component<skinned_mesh_component>(e);
                if (comp)
                {
                    json comp_json = json::object();
                    const aria::asset_guid guid =
                        m_asset_resolver.to_guid<aria::skinned_model_asset>(comp->model_handle);

                    comp_json["model_guid"] = uuids::to_string(guid);
                    comp_json["sub_mesh_index"] = comp->sub_mesh_index;
                    comp_json["cast_shadows"] = comp->cast_shadows;

                    comps_out["skinned_mesh_component"] = comp_json;
                }
            }

            // 12. Skeleton Component
            if (reg.has_component<skeleton_component>(e))
            {
                auto* comp = const_cast<registry&>(reg).get_component<skeleton_component>(e);
                if (comp)
                {
                    json comp_json = json::object();
                    const aria::asset_guid guid =
                        m_asset_resolver.to_guid<aria::skeleton_asset>(comp->skeleton_handle);

                    comp_json["skeleton_guid"] = uuids::to_string(guid);
                    comps_out["skeleton_component"] = comp_json;
                }
            }

            // 13. Animator Component (Aria Animasyon Klibi Entegrasyonu)
            if (reg.has_component<animator_component>(e))
            {
                auto* comp = const_cast<registry&>(reg).get_component<animator_component>(e);
                if (comp)
                {
                    json comp_json = json::object();
                    const aria::asset_guid guid =
                        m_asset_resolver.to_guid<aria::animation_clip_asset>(comp->current_clip);

                    comp_json["clip_guid"] = uuids::to_string(guid);
                    comp_json["playback_time"] = comp->playback_time;
                    comp_json["playback_speed"] = comp->playback_speed;
                    comp_json["is_playing"] = comp->is_playing;
                    comp_json["is_looping"] = comp->is_looping;
                    comp_json["apply_root_motion"] = comp->apply_root_motion;

                    comps_out["animator_component"] = comp_json;
                }
            }

            // 14. Bone Attachment Component (Kemik Soketi)
            if (reg.has_component<bone_attachment_component>(e))
            {
                auto* comp = const_cast<registry&>(reg).get_component<bone_attachment_component>(e);
                if (comp)
                {
                    json comp_json = json::object();
                    std::string target_uuid = "";
                    if (comp->target_skeleton_entity != 0)
                    {
                        target_uuid = m_entity_registry.get_uuid(comp->target_skeleton_entity);
                    }

                    comp_json["target_skeleton_uuid"] = target_uuid;
                    comp_json["bone_name"] = comp->bone_name;
                    comp_json["local_offset"] = { comp->local_offset.x, comp->local_offset.y, comp->local_offset.z };
                    comp_json["local_rotation"] = { comp->local_rotation.x, comp->local_rotation.y, comp->local_rotation.z };
                    comp_json["local_scale"] = { comp->local_scale.x, comp->local_scale.y, comp->local_scale.z };

                    comps_out["bone_attachment_component"] = comp_json;
                }
            }

            scene_json["entities"].push_back(entity_json);
        }

        std::ofstream file(filepath);
        if (!file.is_open())
        {
            report.add_critical("Sahne dosyasi diskte olusturulamadi: " + filepath);
            return report;
        }

        file << scene_json.dump(4);
        file.close();

        report.add_info("Sahne basariyla SERA formatinda kaydedildi: " + filepath);
        return report;
    }

    validation_report sera_session::load(const std::string& filepath, scene& out_scene)
    {
        validation_report report;

        std::ifstream file(filepath);
        if (!file.is_open())
        {
            report.add_critical("Sahne dosyasi diskten okunamadi: " + filepath);
            return report;
        }

        json scene_json;
        try
        {
            file >> scene_json;
        }
        catch (const json::parse_error& e)
        {
            file.close();
            report.add_critical("JSON format hatasi: " + std::string(e.what()));
            return report;
        }
        file.close();

        out_scene.new_scene();
        m_entity_registry.clear();
        registry& reg = out_scene.get_registry();

        // 0. Çevre, Sis ve Renk Ayarlarını (Environment) geri yükle
        if (scene_json.contains("environment"))
        {
            const auto& env = scene_json["environment"];
            if (env.contains("fog"))
            {
                const auto& fogJ = env["fog"];
                m_ctx.fog.enabled = fogJ.value("enabled", true);
                m_ctx.fog.show_in_scene = fogJ.value("show_in_scene", true);
                if (fogJ.contains("color") && fogJ["color"].is_array() && fogJ["color"].size() >= 3)
                {
                    m_ctx.fog.color = glm::vec3(fogJ["color"][0], fogJ["color"][1], fogJ["color"][2]);
                }
                m_ctx.fog.density = fogJ.value("density", 0.008f);
            }
            if (env.contains("rendering"))
            {
                const auto& rendJ = env["rendering"];
                if (rendJ.contains("clear_color_scene") && rendJ["clear_color_scene"].is_array() && rendJ["clear_color_scene"].size() >= 3)
                {
                    m_ctx.renderSettings.clear_color_scene = glm::vec3(rendJ["clear_color_scene"][0], rendJ["clear_color_scene"][1], rendJ["clear_color_scene"][2]);
                }
                if (rendJ.contains("clear_color_game") && rendJ["clear_color_game"].is_array() && rendJ["clear_color_game"].size() >= 3)
                {
                    m_ctx.renderSettings.clear_color_game = glm::vec3(rendJ["clear_color_game"][0], rendJ["clear_color_game"][1], rendJ["clear_color_game"][2]);
                }
            }
        }

        std::unordered_map<std::string, entity> uuid_to_runtime_entity_map;

        if (scene_json.contains("entities"))
        {
            // 1. Aşama: Varlık tanıtıcılarını oluştur ve UUID bağlantılarını kur
            for (const auto& entity_json : scene_json["entities"])
            {
                std::string uuid = entity_json.value("uuid", "");
                if (uuid.empty())
                {
                    uuid = sera_entity_registry::generate_uuid();
                }

                const entity new_e = out_scene.create_entity();
                m_entity_registry.register_link(new_e, uuid);
                uuid_to_runtime_entity_map[uuid] = new_e;
            }

            // 2. Aşama: Bileşenleri ilgili varlıklara deserialize et
            for (const auto& entity_json : scene_json["entities"])
            {
                const std::string uuid = entity_json["uuid"];
                const entity current_e = uuid_to_runtime_entity_map[uuid];

                if (!entity_json.contains("components"))
                {
                    continue;
                }
                const auto& comps_json = entity_json["components"];

                // Transform Component
                if (comps_json.contains("transform_component"))
                {
                    transform_component comp{};
                    deserialize_visitor visitor{ comps_json["transform_component"] };
                    sera_reflect_component(comp, visitor);
                    reg.add_component<transform_component>(current_e, comp);
                }

                // Name Component
                if (comps_json.contains("name_component"))
                {
                    name_component comp{};
                    deserialize_visitor visitor{ comps_json["name_component"] };
                    sera_reflect_component(comp, visitor);
                    reg.add_component<name_component>(current_e, comp);
                }

                // Mesh Component (Aria Model Handle)
                if (comps_json.contains("mesh_component"))
                {
                    mesh_component comp{};

                    const std::string guid_str = comps_json["mesh_component"].value("model_guid", "");
                    const auto parsed = uuids::uuid::from_string(guid_str);

                    if (parsed)
                    {
                        comp.model_handle = m_asset_resolver.to_handle<aria::model_asset>(*parsed);
                    }

                    comp.sub_mesh_index = comps_json["mesh_component"].value("sub_mesh_index", size_t{ 0 });
                    comp.source = static_cast<mesh_source_type>(comps_json["mesh_component"].value("source", 0));

                    reg.add_component<mesh_component>(current_e, comp);
                }

                // Material Component (Aria Material Handle)
                if (comps_json.contains("material_component"))
                {
                    material_component comp{};

                    const std::string guid_str = comps_json["material_component"].value("material_guid", "");
                    const auto parsed = uuids::uuid::from_string(guid_str);

                    if (parsed)
                    {
                        comp.material_handle = m_asset_resolver.to_handle<aria::material_asset>(*parsed);
                    }

                    reg.add_component<material_component>(current_e, comp);
                }

                // Camera Component
                if (comps_json.contains("camera_component"))
                {
                    camera_component comp{};
                    deserialize_visitor visitor{ comps_json["camera_component"] };
                    sera_reflect_component(comp, visitor);
                    reg.add_component<camera_component>(current_e, comp);
                }

                // Light Component
                if (comps_json.contains("light_component"))
                {
                    light_component comp{};
                    deserialize_visitor visitor{ comps_json["light_component"] };
                    sera_reflect_component(comp, visitor);
                    reg.add_component<light_component>(current_e, comp);
                }

                // Bounds Component
                if (comps_json.contains("bounds_component"))
                {
                    bounds_component comp{};
                    deserialize_visitor visitor{ comps_json["bounds_component"] };
                    sera_reflect_component(comp, visitor);
                    reg.add_component<bounds_component>(current_e, comp);
                }

                // Rigidbody Component
                if (comps_json.contains("rigidbody_component"))
                {
                    rigidbody_component comp{};
                    deserialize_visitor visitor{ comps_json["rigidbody_component"] };
                    sera_reflect_component(comp, visitor);
                    reg.add_component<rigidbody_component>(current_e, comp);
                }

                // Collider Component
                if (comps_json.contains("collider_component"))
                {
                    collider_component comp{};
                    deserialize_visitor visitor{ comps_json["collider_component"] };
                    sera_reflect_component(comp, visitor);
                    reg.add_component<collider_component>(current_e, comp);
                }

                // Native Script Component
                if (comps_json.contains("native_script_component"))
                {
                    const auto& sc = comps_json["native_script_component"];
                    const std::string name = sc.value("script_name", "None");

                    if (!name.empty() && name != "None")
                    {
                        auto& comp = reg.add_component<native_script_component>(current_e);
                        comp.script_name = name;
                        script_factory::instantiate_script(name, current_e, reg);
                    }
                }

                // 11. Skinned Mesh Component
                if (comps_json.contains("skinned_mesh_component"))
                {
                    skinned_mesh_component comp{};

                    const std::string guid_str = comps_json["skinned_mesh_component"].value("model_guid", "");
                    const auto parsed = uuids::uuid::from_string(guid_str);

                    if (parsed)
                    {
                        comp.model_handle = m_asset_resolver.to_handle<aria::skinned_model_asset>(*parsed);
                    }

                    comp.sub_mesh_index = comps_json["skinned_mesh_component"].value("sub_mesh_index", size_t{ 0 });
                    comp.cast_shadows = comps_json["skinned_mesh_component"].value("cast_shadows", true);

                    reg.add_component<skinned_mesh_component>(current_e, comp);

                    // Eğer model yüklendiyse ve entity'de henüz skeleton_component yoksa, modelden otomatik bağla
                    if (comp.model_handle.is_valid() && m_ctx.aria_system)
                    {
                        const auto* modelData = m_ctx.aria_system->get_skinned_model_data(comp.model_handle);
                        if (modelData && modelData->skeleton_handle.is_valid())
                        {
                            if (!reg.has_component<skeleton_component>(current_e))
                            {
                                auto& skelComp = reg.add_component<skeleton_component>(current_e);
                                skelComp.skeleton_handle = modelData->skeleton_handle;
                                skelComp.is_dirty = true;
                            }
                        }
                    }
                }

                // 12. Skeleton Component
                if (comps_json.contains("skeleton_component"))
                {
                    skeleton_component comp{};

                    const std::string guid_str = comps_json["skeleton_component"].value("skeleton_guid", "");
                    const auto parsed = uuids::uuid::from_string(guid_str);

                    if (parsed)
                    {
                        comp.skeleton_handle = m_asset_resolver.to_handle<aria::skeleton_asset>(*parsed);
                    }
                    comp.is_dirty = true;

                    if (reg.has_component<skeleton_component>(current_e))
                    {
                        auto* existing = reg.get_component<skeleton_component>(current_e);
                        if (comp.skeleton_handle.is_valid())
                        {
                            existing->skeleton_handle = comp.skeleton_handle;
                        }
                    }
                    else
                    {
                        reg.add_component<skeleton_component>(current_e, comp);
                    }
                }

                // 13. Animator Component
                if (comps_json.contains("animator_component"))
                {
                    animator_component comp{};

                    const std::string guid_str = comps_json["animator_component"].value("clip_guid", "");
                    const auto parsed = uuids::uuid::from_string(guid_str);

                    if (parsed)
                    {
                        comp.current_clip = m_asset_resolver.to_handle<aria::animation_clip_asset>(*parsed);
                    }

                    comp.playback_time = comps_json["animator_component"].value("playback_time", 0.0f);
                    comp.playback_speed = comps_json["animator_component"].value("playback_speed", 1.0f);
                    comp.is_playing = comps_json["animator_component"].value("is_playing", true);
                    comp.is_looping = comps_json["animator_component"].value("is_looping", true);
                    comp.apply_root_motion = comps_json["animator_component"].value("apply_root_motion", false);

                    reg.add_component<animator_component>(current_e, comp);
                }

                // 14. Bone Attachment Component
                if (comps_json.contains("bone_attachment_component"))
                {
                    bone_attachment_component comp{};

                    const std::string target_uuid = comps_json["bone_attachment_component"].value("target_skeleton_uuid", "");
                    if (!target_uuid.empty())
                    {
                        auto it = uuid_to_runtime_entity_map.find(target_uuid);
                        if (it != uuid_to_runtime_entity_map.end())
                        {
                            comp.target_skeleton_entity = it->second;
                        }
                    }

                    comp.bone_name = comps_json["bone_attachment_component"].value("bone_name", "");

                    if (comps_json["bone_attachment_component"].contains("local_offset"))
                    {
                        const auto& v = comps_json["bone_attachment_component"]["local_offset"];
                        if (v.is_array() && v.size() == 3)
                            comp.local_offset = glm::vec3(v[0].get<float>(), v[1].get<float>(), v[2].get<float>());
                    }
                    if (comps_json["bone_attachment_component"].contains("local_rotation"))
                    {
                        const auto& v = comps_json["bone_attachment_component"]["local_rotation"];
                        if (v.is_array() && v.size() == 3)
                            comp.local_rotation = glm::vec3(v[0].get<float>(), v[1].get<float>(), v[2].get<float>());
                    }
                    if (comps_json["bone_attachment_component"].contains("local_scale"))
                    {
                        const auto& v = comps_json["bone_attachment_component"]["local_scale"];
                        if (v.is_array() && v.size() == 3)
                            comp.local_scale = glm::vec3(v[0].get<float>(), v[1].get<float>(), v[2].get<float>());
                    }

                    reg.add_component<bone_attachment_component>(current_e, comp);
                }
            }
        }

        // Aktif kameranın geri yüklenmesi
        if (scene_json.contains("active_camera_uuid"))
        {
            const std::string cam_uuid = scene_json["active_camera_uuid"];
            if (!cam_uuid.empty())
            {
                const auto it = uuid_to_runtime_entity_map.find(cam_uuid);
                if (it != uuid_to_runtime_entity_map.end())
                {
                    out_scene.set_active_camera(it->second);
                }
            }
        }

        out_scene.update_light_batch();
        report.add_info("Sahne ve bagli Aria varlik referanslari basariyla geri yuklendi.");
        return report;
    }

} // namespace sera