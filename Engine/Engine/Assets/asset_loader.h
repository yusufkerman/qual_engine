#pragma once

#include "asset_raw_structures.h"
#include <optional>
#include <string>

namespace aria {

    class asset_loader {
    public:
        [[nodiscard]] static std::optional<raw_texture_data> load_texture_to_cpu(const std::string& file_path);
        [[nodiscard]] static std::optional<raw_model_data> load_model_to_cpu(const std::string& file_path);
        [[nodiscard]] static std::optional<raw_material_data> load_material_to_cpu(const std::string& file_path);
        [[nodiscard]] static std::optional<raw_shader_data> load_shader_to_cpu(const std::string& file_path);

    };

} // namespace aria