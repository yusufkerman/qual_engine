// gpu_resource_cache.h
#pragma once
#include "asset_registry.h"
#include "render_backend.h"
#include <map>

namespace aria {

    class gpu_resource_cache {
    private:
        asset_registry* m_registry{ nullptr };
        render_backend* m_backend{ nullptr }; // 🔥 Soyut backend bağımlılığı

        std::map<handle<texture_asset>, gpu_texture_id> m_texture_cache;
        std::map<handle<model_asset>, std::vector<gpu_mesh_id>> m_model_cache;

        gpu_texture_id m_fallback_texture_id{ 0 };

        std::map<handle<shader_asset>, gpu_shader_id> m_shader_cache;
        gpu_shader_id  m_fallback_shader_id{ 0 };

    public:
        gpu_resource_cache(asset_registry* registry, render_backend* backend);
        ~gpu_resource_cache();

        void init();
        void shutdown();

        gpu_texture_id get_texture_id(handle<texture_asset> h);
        gpu_mesh_id get_mesh_gpu_id(handle<model_asset> h, size_t sub_mesh_index);
        gpu_shader_id get_shader_id(handle<shader_asset> h);

        void evict_texture(handle<texture_asset> h);
        void evict_model(handle<model_asset> h);
        void evict_shader(handle<shader_asset> h);

        void clear_gpu_cache();

    };

} // namespace aria