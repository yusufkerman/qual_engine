// opengl_backend.h
#pragma once
#include "render_backend.h"

namespace aria {

    class opengl_backend : public render_backend {
    public:
        void init() override;
        void shutdown() override;

        gpu_texture_id create_texture(const raw_texture_data& raw_data) override;
        std::vector<gpu_mesh_id> create_model(const raw_model_data& raw_data) override;
        gpu_texture_id create_fallback_texture() override;
        gpu_shader_id create_shader(const raw_shader_data& raw_data) override;
        gpu_shader_id create_fallback_shader() override;

        void destroy_texture(gpu_texture_id id) override;
        void destroy_model(const std::vector<gpu_mesh_id>& mesh_ids) override;
        void destroy_shader(gpu_shader_id id) override;

        void reset_state() override;

    };

} // namespace aria