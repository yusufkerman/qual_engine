/**
 * @file aria.cpp
 * @brief Aria sisteminin mantıksal işlevlerini ve alt sistemler arası koordinasyonunu içerir.
 */

#include "aria.h"
#include <iostream>

namespace aria {

    static const uuids::uuid aria_namespace_uuid = 
        uuids::uuid::from_string("6ba7b810-9dad-11d1-80b4-00c04fd430c8").value();

    system::system()
        : m_database()
        , m_registry(&m_database)
        , m_backend()
        , m_cache(&m_registry, &m_backend)
    {
    }

    system::~system() 
    { 
        shutdown(); 
    }

    void system::init() 
    {
        if (m_database.load_from_disk("asset_manifest.json")) 
        {
            std::cout << "[ARIA System] asset_manifest.json basariyla yuklendi.\n";
        }
        else 
        {
            std::cout << "[ARIA System] Uyari: asset_manifest.json bulunamadi, yeni olusturulacak.\n";
        }

        m_cache.init();
    }

    void system::shutdown() 
    {
        save_asset_database();
        m_cache.shutdown();
    }

    void system::register_asset_path(const asset_guid& guid, const std::string& file_path) 
    {
        m_database.register_asset(guid, file_path);
    }

    void system::save_asset_database() 
    {
        if (m_database.save_to_disk("asset_manifest.json")) 
        {
            std::cout << "[ARIA System] Adres defteri diske basariyla senkronize edildi.\n";
        }
        else 
        {
            std::cout << "[ARIA System] Hata: Adres defteri diske yazilamadi!\n";
        }
    }

    std::string system::resolve_asset_path(const asset_guid& guid) const 
    {
        return m_database.resolve_path(guid);
    }

    handle<texture_asset> system::import_texture(const asset_guid& guid, const std::string& file_path) 
    {
        register_asset_path(guid, file_path);
        return m_registry.load_texture(guid);
    }

    handle<model_asset> system::import_model(const asset_guid& guid, const std::string& file_path) 
    {
        register_asset_path(guid, file_path);
        return m_registry.load_model(guid, file_path);
    }

    handle<skinned_model_asset> system::import_skinned_model(const asset_guid& guid, const std::string& file_path)
    {
        register_asset_path(guid, file_path);
        return m_registry.load_skinned_model(guid, file_path);
    }

    handle<skeleton_asset> system::import_skeleton(const asset_guid& guid, const std::string& file_path)
    {
        register_asset_path(guid, file_path);
        return m_registry.load_skeleton(guid, file_path);
    }

    handle<animation_clip_asset> system::import_animation(const asset_guid& guid, const std::string& file_path)
    {
        register_asset_path(guid, file_path);
        return m_registry.load_animation(guid, file_path);
    }

    handle<material_asset> system::import_material(const asset_guid& guid, const std::string& file_path) 
    {
        register_asset_path(guid, file_path);
        return m_registry.load_material(guid, file_path);
    }

    handle<shader_asset> system::import_shader(const asset_guid& guid, const std::string& file_path) 
    {
        register_asset_path(guid, file_path);
        return m_registry.load_shader(guid);
    }

    handle<model_asset> system::import_virtual_model(const std::string& file_path) 
    {
        uuids::uuid_name_generator gen(aria_namespace_uuid);
        asset_guid virtual_guid = gen(file_path);

        return import_model(virtual_guid, file_path);
    }

    const material_asset* system::get_material_data(handle<material_asset> h) 
    { 
        return m_registry.get_material(h); 
    }
    
    gpu_texture_id system::get_texture_id(handle<texture_asset> h) 
    { 
        return m_cache.get_texture_id(h); 
    }
    
    gpu_shader_id system::get_shader_id(handle<shader_asset> h) 
    { 
        return m_cache.get_shader_id(h); 
    }

    submesh_runtime_info system::get_submesh_info(handle<model_asset> h, size_t sub_mesh_index) 
    {
        return m_cache.get_submesh_info(h, sub_mesh_index);
    }

    size_t system::get_mesh_index_count(handle<model_asset> h, size_t sub_mesh_index) 
    {
        const model_asset* model = m_registry.get_model(h);
        if (model && sub_mesh_index < model->sub_meshes.size()) 
        {
            return model->sub_meshes[sub_mesh_index].index_count;
        }
        return 0;
    }

    submesh_runtime_info system::get_skinned_submesh_info(handle<skinned_model_asset> h, size_t sub_mesh_index)
    {
        return m_cache.get_skinned_submesh_info(h, sub_mesh_index);
    }

    size_t system::get_skinned_mesh_index_count(handle<skinned_model_asset> h, size_t sub_mesh_index)
    {
        const skinned_model_asset* model = m_registry.get_skinned_model(h);
        if (model && sub_mesh_index < model->sub_meshes.size())
        {
            return model->sub_meshes[sub_mesh_index].index_count;
        }
        return 0;
    }

    const skeleton_asset* system::get_skeleton_data(handle<skeleton_asset> h)
    {
        return m_registry.get_skeleton(h);
    }

    const animation_clip_asset* system::get_animation_data(handle<animation_clip_asset> h)
    {
        return m_registry.get_animation(h);
    }

    const skinned_model_asset* system::get_skinned_model_data(handle<skinned_model_asset> h)
    {
        return m_registry.get_skinned_model(h);
    }

    void system::unload_texture(handle<texture_asset> h) 
    { 
        m_cache.evict_texture(h); 
        m_registry.destroy_texture(h); 
    }
    
    void system::unload_model(handle<model_asset> h) 
    { 
        m_cache.evict_model(h); 
        m_registry.destroy_model(h); 
    }

    void system::unload_skinned_model(handle<skinned_model_asset> h)
    {
        m_cache.evict_skinned_model(h);
        m_registry.destroy_skinned_model(h);
    }

    void system::unload_skeleton(handle<skeleton_asset> h)
    {
        m_registry.destroy_skeleton(h);
    }

    void system::unload_animation(handle<animation_clip_asset> h)
    {
        m_registry.destroy_animation(h);
    }
    
    void system::unload_shader(handle<shader_asset> h) 
    { 
        m_cache.evict_shader(h); 
        m_registry.destroy_shader(h); 
    }

    void system::clear_all_resources() 
    {
        m_cache.clear_gpu_cache();
        m_registry.clear_registry();
        m_backend.reset_state();
    }

} // namespace aria