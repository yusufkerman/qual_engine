#pragma once

#include "asset_raw_structures.h"
#include <vector>

namespace aria {

    // API bağımsız GPU kaynak kimlikleri (Handles)
    using gpu_texture_id = unsigned int;

    using gpu_shader_id = unsigned int;

    struct gpu_mesh_id {
        unsigned int vao_or_buffer_id{ 0 };
        unsigned int vbo{ 0 };
        unsigned int ebo{ 0 };
    };

    class render_backend {
    public:
        virtual ~render_backend() = default;

        virtual void init() = 0;
        virtual void shutdown() = 0;

        // Soyut Yükleme Fonksiyonları
        [[nodiscard]] virtual gpu_texture_id create_texture(const raw_texture_data& raw_data) = 0;
        [[nodiscard]] virtual std::vector<gpu_mesh_id> create_model(const raw_model_data& raw_data) = 0;
        [[nodiscard]] virtual gpu_texture_id create_fallback_texture() = 0;
        [[nodiscard]] virtual gpu_shader_id create_shader(const raw_shader_data& raw_data) = 0;
        [[nodiscard]] virtual gpu_shader_id create_fallback_shader() = 0;

        // Soyut Silme Fonksiyonları
        virtual void destroy_texture(gpu_texture_id id) = 0;
        virtual void destroy_model(const std::vector<gpu_mesh_id>& mesh_ids) = 0;
        virtual void destroy_shader(gpu_shader_id id) = 0;


        virtual void reset_state() = 0;


    };

} // namespace aria