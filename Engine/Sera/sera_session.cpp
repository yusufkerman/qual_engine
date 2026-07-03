#include "sera_session.h"
#include "sera_component_codec.h"
#include "../Engine/engine_context.h"
#include "../ECS/components.h"       
#include "../Engine/Assets/aria.h"
#include <fstream>
#include <iostream>
#include "../Engine/Scripting/script_factory.h"

// NOT: DESCRIBE_COMPONENT makro mühürleri components.h dosyasının altında olduğu için buradan kaldırıldı.

namespace sera {

    sera_session::sera_session(EngineContext& ctx)
        : m_ctx(ctx), m_asset_resolver(ctx)
    {
    }

    // ============================================================================
    // 💾 SAHNE KAYDETME MOTORU (SAVE)
    // ============================================================================
    validation_report sera_session::save(const std::string& filepath, const scene& current_scene) {
        validation_report report;
        const registry& reg = current_scene.get_registry();
        const auto& all_entities = reg.get_all_entities();

        json scene_json;
        scene_json["scene_name"] = "Sera_Engine_Scene";

        entity active_cam = current_scene.get_active_camera();
        if (active_cam != 0) {
            scene_json["active_camera_uuid"] = m_entity_registry.get_uuid(active_cam);
        }
        else {
            scene_json["active_camera_uuid"] = "";
        }

        scene_json["entities"] = json::array();

        for (entity e : all_entities) {
            json entity_json;

            std::string ent_uuid = m_entity_registry.get_uuid(e);
            if (ent_uuid.empty()) {
                ent_uuid = sera_entity_registry::generate_uuid();
                m_entity_registry.register_link(e, ent_uuid);
            }
            entity_json["uuid"] = ent_uuid;

            entity_json["components"] = json::object();
            json& comps_out = entity_json["components"];

            // 1. Name Component
            if (reg.has_component<name_component>(e)) {
                auto* comp = const_cast<registry&>(reg).get_component<name_component>(e);
                if (comp) {
                    json comp_json = json::object();
                    serialize_visitor visitor{ comp_json };
                    sera_reflect_component(*comp, visitor);
                    comps_out["name_component"] = comp_json;
                }
            }

            // 2. Transform Component
            if (reg.has_component<transform_component>(e)) {
                auto* comp = const_cast<registry&>(reg).get_component<transform_component>(e);
                if (comp) {
                    json comp_json = json::object();
                    serialize_visitor visitor{ comp_json };
                    sera_reflect_component(*comp, visitor);
                    comps_out["transform_component"] = comp_json;
                }
            }

            // 3. Mesh Component (Aria Entegrasyonu + sub_mesh_index ve source alanları)
            if (reg.has_component<mesh_component>(e)) {
                auto* comp = const_cast<registry&>(reg).get_component<mesh_component>(e);
                if (comp) {
                    json comp_json = json::object();

                    aria::asset_guid guid =
                        m_asset_resolver.to_guid<aria::model_asset>(comp->model_handle);

                    comp_json["model_guid"] = uuids::to_string(guid);
                    comp_json["sub_mesh_index"] = comp->sub_mesh_index;
                    comp_json["source"] = static_cast<int>(comp->source);

                    comps_out["mesh_component"] = comp_json;
                }
            }

            // 4. Material Component (Aria Entegrasyonu)
            if (reg.has_component<material_component>(e)) {
                auto* comp = const_cast<registry&>(reg).get_component<material_component>(e);
                if (comp) {
                    json comp_json = json::object();

                    aria::asset_guid guid =
                        m_asset_resolver.to_guid<aria::material_asset>(comp->material_handle);

                    comp_json["material_guid"] = uuids::to_string(guid);

                    comps_out["material_component"] = comp_json;
                }
            }

            // 5. Camera Component
            if (reg.has_component<camera_component>(e)) {
                auto* comp = const_cast<registry&>(reg).get_component<camera_component>(e);
                if (comp) {
                    json comp_json = json::object();
                    serialize_visitor visitor{ comp_json };
                    sera_reflect_component(*comp, visitor);
                    comps_out["camera_component"] = comp_json;
                }
            }

            // 6. Light Component
            if (reg.has_component<light_component>(e)) {
                auto* comp = const_cast<registry&>(reg).get_component<light_component>(e);
                if (comp) {
                    json comp_json = json::object();
                    serialize_visitor visitor{ comp_json };
                    sera_reflect_component(*comp, visitor);
                    comps_out["light_component"] = comp_json;
                }
            }

            // 7. Bounds Component
            if (reg.has_component<bounds_component>(e)) {
                auto* comp = const_cast<registry&>(reg).get_component<bounds_component>(e);
                if (comp) {
                    json comp_json = json::object();
                    serialize_visitor visitor{ comp_json };
                    sera_reflect_component(*comp, visitor);
                    comps_out["bounds_component"] = comp_json;
                }
            }

            // 8. Rigidbody Component (Physics)
            if (reg.has_component<rigidbody_component>(e)) {
                auto* comp = const_cast<registry&>(reg).get_component<rigidbody_component>(e);
                if (comp) {
                    json comp_json = json::object();
                    serialize_visitor visitor{ comp_json };
                    sera_reflect_component(*comp, visitor); // BodyID hariç ayarları otomatik paketler
                    comps_out["rigidbody_component"] = comp_json;
                }
            }

            // 9. Collider Component
            if (reg.has_component<collider_component>(e)) {
                auto* comp = const_cast<registry&>(reg).get_component<collider_component>(e);
                if (comp) {
                    json comp_json = json::object();
                    serialize_visitor visitor{ comp_json };
                    sera_reflect_component(*comp, visitor);
                    comps_out["collider_component"] = comp_json;
                }
            }

            // 10. Native Script Component
            if (reg.has_component<native_script_component>(e)) {
                auto* comp = const_cast<registry&>(reg).get_component<native_script_component>(e);
                if (comp) {
                    json comp_json = json::object();
                    serialize_visitor visitor{ comp_json };
                    sera_reflect_component(*comp, visitor);
                    comps_out["native_script_component"] = comp_json;
                }
            }

            scene_json["entities"].push_back(entity_json);
        }

        std::ofstream file(filepath);
        if (!file.is_open()) {
            report.add_critical("Sahne dosyasi diskte olusturulamadi: " + filepath);
            return report;
        }

        file << scene_json.dump(4);
        file.close();

        report.add_info("Sahne basariyla SERA formatinda mühürlendi: " + filepath);
        return report;
    }

    // ============================================================================
    // 📂 SAHNE YÜKLEME MOTORU (LOAD)
    // ============================================================================
    validation_report sera_session::load(const std::string& filepath, scene& out_scene) {
        validation_report report;

        std::ifstream file(filepath);
        if (!file.is_open()) {
            report.add_critical("Sahne dosyasi diskten okunmadi: " + filepath);
            return report;
        }

        json scene_json;
        try { file >> scene_json; }
        catch (const json::parse_error& e) {
            file.close();
            report.add_critical("JSON format hatasi: " + std::string(e.what()));
            return report;
        }
        file.close();

        out_scene.new_scene();
        m_entity_registry.clear();
        registry& reg = out_scene.get_registry();

        std::unordered_map<std::string, entity> uuid_to_runtime_entity_map;

        if (scene_json.contains("entities")) {
            for (const auto& entity_json : scene_json["entities"]) {
                std::string uuid = entity_json.value("uuid", "");
                if (uuid.empty()) {
                    uuid = sera_entity_registry::generate_uuid();
                }

                entity new_e = out_scene.create_entity();
                m_entity_registry.register_link(new_e, uuid);
                uuid_to_runtime_entity_map[uuid] = new_e;
            }

            for (const auto& entity_json : scene_json["entities"]) {
                std::string uuid = entity_json["uuid"];
                entity current_e = uuid_to_runtime_entity_map[uuid];

                if (!entity_json.contains("components")) continue;
                const auto& comps_json = entity_json["components"];

                // 1. Transform Component Yükleme
                if (comps_json.contains("transform_component")) {
                    transform_component comp{};
                    deserialize_visitor visitor{ comps_json["transform_component"] };
                    sera_reflect_component(comp, visitor);
                    reg.add_component<transform_component>(current_e, comp);
                }

                // 2. Name Component Yükleme
                if (comps_json.contains("name_component")) {
                    name_component comp{};
                    deserialize_visitor visitor{ comps_json["name_component"] };
                    sera_reflect_component(comp, visitor);
                    reg.add_component<name_component>(current_e, comp);
                }

                // 3. Mesh Component Yükleme
                if (comps_json.contains("mesh_component")) {
                    mesh_component comp{};

                    std::string guid_str =
                        comps_json["mesh_component"].value("model_guid", "");

                    auto parsed = uuids::uuid::from_string(guid_str);

                    if (parsed)
                    {
                        comp.model_handle =
                            m_asset_resolver.to_handle<aria::model_asset>(*parsed);
                    }

                    comp.sub_mesh_index =
                        comps_json["mesh_component"].value("sub_mesh_index", size_t{ 0 });

                    comp.source =
                        static_cast<mesh_source_type>(
                            comps_json["mesh_component"].value("source", 0));

                    reg.add_component<mesh_component>(current_e, comp);
                }

                // 4. Material Component Yükleme
                if (comps_json.contains("material_component")) {
                    material_component comp{};

                    std::string guid_str =
                        comps_json["material_component"].value("material_guid", "");

                    auto parsed = uuids::uuid::from_string(guid_str);

                    if (parsed)
                    {
                        comp.material_handle =
                            m_asset_resolver.to_handle<aria::material_asset>(*parsed);
                    }

                    reg.add_component<material_component>(current_e, comp);
                }

                // 5. Camera Component Yükleme
                if (comps_json.contains("camera_component")) {
                    camera_component comp{};
                    deserialize_visitor visitor{ comps_json["camera_component"] };
                    sera_reflect_component(comp, visitor);
                    reg.add_component<camera_component>(current_e, comp);
                }

                // 6. Light Component Yükleme
                if (comps_json.contains("light_component")) {
                    light_component comp{};
                    deserialize_visitor visitor{ comps_json["light_component"] };
                    sera_reflect_component(comp, visitor);
                    reg.add_component<light_component>(current_e, comp);
                }

                // 7. Bounds Component Yükleme
                if (comps_json.contains("bounds_component")) {
                    bounds_component comp{};
                    deserialize_visitor visitor{ comps_json["bounds_component"] };
                    sera_reflect_component(comp, visitor);
                    reg.add_component<bounds_component>(current_e, comp);
                }

                // 8. Rigidbody Component Yükleme (Physics)
                if (comps_json.contains("rigidbody_component")) {
                    rigidbody_component comp{};
                    deserialize_visitor visitor{ comps_json["rigidbody_component"] };
                    sera_reflect_component(comp, visitor);
                    // Not: JPH::BodyID simülasyon başladığında Jolt tarafından atanacağı için burada sıfır kalır, sorun yok.
                    reg.add_component<rigidbody_component>(current_e, comp);
                }

                // 9. Collider Component Yükleme
                if (comps_json.contains("collider_component")) {
                    collider_component comp{};
                    deserialize_visitor visitor{ comps_json["collider_component"] };
                    sera_reflect_component(comp, visitor);
                    reg.add_component<collider_component>(current_e, comp);
                }

                // 10. Native Script Component Yükleme
                if (comps_json.contains("native_script_component")) {

                    auto& sc = comps_json["native_script_component"];

                    std::string name = sc.value("script_name", "None");

                    if (!name.empty() && name != "None") {

                        auto& comp = reg.add_component<native_script_component>(current_e);
                        comp.script_name = name;

                        // sonra factory
                        script_factory::instantiate_script(name, current_e, reg);
                    }
                }
            }
        }

        if (scene_json.contains("active_camera_uuid")) {
            std::string cam_uuid = scene_json["active_camera_uuid"];
            if (!cam_uuid.empty() && uuid_to_runtime_entity_map.count(cam_uuid)) {
                out_scene.set_active_camera(uuid_to_runtime_entity_map[cam_uuid]);
            }
        }

        out_scene.update_light_batch();
        report.add_info("Sahne ve tüm ARIA handle baglantilari basariyla geri yüklendi.");
        return report;
    }

} // namespace sera